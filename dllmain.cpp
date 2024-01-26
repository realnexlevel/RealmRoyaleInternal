#include <Windows.h>
#include <type_traits>
#include <string>
#include <vector>
#include <cmath>
#include <iostream>
#include <optional>
#include <chrono>
#include <thread>

#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_internal.h"

#include "definitions.hpp"
#include "utils.hpp"

#include "minhook/include/MinHook.h"

IMGUI_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

ImGuiWindow& BeginScene()
{
	ImGui_ImplDX11_NewFrame();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
	ImGui::Begin("##scene", nullptr, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar);

	auto& io = ImGui::GetIO();
	ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y), ImGuiCond_Always);

	return *ImGui::GetCurrentWindow();
}

ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;
ImGuiColorEditFlags ColorPickerFlags = ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs;

auto EndScene(ImGuiWindow& window) -> void
{
	window.DrawList->PushClipRectFullScreen();
	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);

	if (Settings::bOptionsEnabled)
	{
		if (ImGui::Begin("Realm Royale", nullptr, WindowFlags))
		{
			ImGui::SetWindowSize(ImVec2(300.0f, 325.0f));

			static auto CurrentSelectedMenuTab = 0;
			if (ImGui::Button("Targeting"))
				CurrentSelectedMenuTab = 0;

			ImGui::SameLine();
			if (ImGui::Button("Player"))
				CurrentSelectedMenuTab = 1;

			ImGui::SameLine();
			if (ImGui::Button("Misc"))
				CurrentSelectedMenuTab = 2;

			switch (CurrentSelectedMenuTab)
			{
			case 0:
				ImGui::NewLine();
				ImGui::Text("Spray when using silent aiming.");
				ImGui::Text("First silent shot may not always.");
				ImGui::NewLine();

				ImGui::Checkbox("Enable", &Settings::bAimbot);
				ImGui::Checkbox("Vischeck", &Settings::bTargetVischeck);
				ImGui::Checkbox("Draw FOV", &Settings::bDrawFOV);
				ImGui::Checkbox("Draw Crosshair", &Settings::bDrawCrosshair);

				if (!strcmp(SelectedAimbot, "MEMORY"))
				{
					ImGui::NewLine();
					Util::GetTargetingButton();
				}

				ImGui::NewLine();

				if (ImGui::BeginCombo("Hitbox", SelectedHitbox))
				{
					for (int n = 0; n < IM_ARRAYSIZE(AllHitboxes); n++)
					{
						auto IsSelected = (SelectedHitbox == AllHitboxes[n]);

						if (ImGui::Selectable(AllHitboxes[n], IsSelected))
							SelectedHitbox = AllHitboxes[n];

						if (IsSelected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}

				if (ImGui::BeginCombo("Type", SelectedAimbot))
				{
					for (int n = 0; n < IM_ARRAYSIZE(AllAimbots); n++)
					{
						auto IsSelected = (SelectedAimbot == AllAimbots[n]);

						if (ImGui::Selectable(AllAimbots[n], IsSelected))
							SelectedAimbot = AllAimbots[n];

						if (IsSelected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}

				ImGui::SliderFloat("FOV", &Settings::TargetingFOV, 0.0f, 1000.0f);
				if (!strcmp(SelectedAimbot, "MEMORY"))
				{
					ImGui::NewLine();
					ImGui::Text("Zero is default hard-lock");
					ImGui::Text("Lower speed is smoother.");
					ImGui::SliderFloat("Speed", &Settings::TargetingSpeed, 0, 25);
				}
				break;
			case 1:

				ImGui::NewLine();

				ImGui::Checkbox("Box", &Settings::bPlayerBox);
				if (Settings::bPlayerBox)
					ImGui::Combo("Style", &SelectedBoxStyle, AllBoxStyles, IM_ARRAYSIZE(AllBoxStyles));

				ImGui::Checkbox("Line", &Settings::bPlayerLine);
				ImGui::Checkbox("Head", &Settings::bPlayerHead);
				ImGui::Checkbox("Distance", &Settings::bPlayerDistance);
				ImGui::Checkbox("Skeleton", &Settings::bPlayerSkeleton);
				break;
			case 2:
				ImGui::NewLine();

				ImGui::Text("RISKY");

				ImGui::NewLine();

				ImGui::Checkbox("Local Speedhack (F2)", &Settings::bLocalSpeedhack);
				ImGui::SliderFloat("Value", &Settings::SpeedHackValue, 1, 100);

				ImGui::NewLine();
				ImGui::Text("Camera FOV (0 = default)");
				ImGui::SliderFloat("    ", &Settings::CameraFOV, 0, 180);

				ImGui::NewLine();

				ImGui::Checkbox("No Spread", &Settings::bNoSpread);
				ImGui::Checkbox("No Recoil", &Settings::bNoRecoil);
				ImGui::Checkbox("Instant Mount", &Settings::bInstantMount);

				ImGui::NewLine();

				ImGui::ColorPicker3("Visible", Settings::PlayerVisibleColor, ColorPickerFlags);
				ImGui::SameLine();
				ImGui::ColorPicker3("Invisible", Settings::PlayerInvisibleColor, ColorPickerFlags);
				break;
			default:
				break;
			}

			ImGui::End();
		}
	}

	ImGui::Render();
}

LRESULT CALLBACK WndProcHook(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_KEYUP && (wParam == VK_INSERT))
	{
		Settings::bOptionsEnabled = !Settings::bOptionsEnabled;

		ImGui::GetIO().MouseDrawCursor = Settings::bOptionsEnabled;
	}
	else if (msg == WM_SIZE)
	{
		bIsResizing = true;

		if (g_pSwapChain && immediateContext)
		{
			immediateContext->OMSetRenderTargets(0, 0, 0);

			// Release all outstanding references to the swap chain's buffers.
			renderTargetView->Release();

			HRESULT hr{};
			// Preserve the existing buffer count and format.
			// Automatically choose the width and height to match the client rect for HWNDs.
			hr = g_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

			// Get buffer and create a render-target-view.
			ID3D11Texture2D* pBuffer;
			hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
				(void**)&pBuffer);

			if (pBuffer)
			{
				hr = device->CreateRenderTargetView(pBuffer, NULL,
					&renderTargetView);

				// Perform error handling here!
				pBuffer->Release();
			}

			immediateContext->OMSetRenderTargets(1, &renderTargetView, NULL);

			// Set up the viewport.
			D3D11_VIEWPORT vp{};
			vp.Width = width;
			vp.Height = height;
			vp.MinDepth = 0.0f;
			vp.MaxDepth = 1.0f;
			vp.TopLeftX = 0;
			vp.TopLeftY = 0;

			immediateContext->RSSetViewports(1, &vp);
		}

		bIsResizing = false;
	}

	if (Settings::bOptionsEnabled)
	{
		ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
		return true; // We don't want the input that's meant for the cheat menu to register in-game...
	}

	return CallWindowProcW(WndProcOriginal, hWnd, msg, wParam, lParam);
}

AWorldInfo* WorldInfo = nullptr;
APlayerController* LocalPlayerController = nullptr;
APawn* LocalPawn = nullptr, *ClosestEnemy = nullptr;

auto OldRotation = FRotator();
auto bLockedCamera = false;

auto bIsShooting = false;

void(*FillCameraCacheOriginal)(ACamera*, ACamera::FTPOV*) = nullptr;
auto FillCameraCacheHook(ACamera* Camera, ACamera::FTPOV* NewPOV) -> void
{
	if (NewPOV != nullptr)
	{
		if (LocalPawn != nullptr && Settings::CameraFOV)
			NewPOV->FOV = Settings::CameraFOV;

		if (!strcmp(SelectedAimbot, "SILENT") && Settings::bAimbot)
		{
			// Silently lock camera while targeting...
			if (LocalPawn != nullptr && OldRotation != FRotator())
				NewPOV->Rotation = OldRotation, bLockedCamera = true;
			else if (bLockedCamera && OldRotation == FRotator())
				bLockedCamera = false;
		}

		// Cache camera data for projection purposes...
		auto CameraLocation = NewPOV->Location;
		if (CameraLocation != FVector())
			ACameraCache::SetLocation(CameraLocation);

		auto CameraRotation = NewPOV->Rotation;
		if (CameraRotation != FRotator())
			ACameraCache::SetRotation(CameraRotation);

		auto CameraFOV = NewPOV->FOV;
		if (CameraFOV > 0.0f)
			ACameraCache::SetFOVAngle(CameraFOV);
	}

	FillCameraCacheOriginal(Camera, NewPOV);
}

struct PlayerInfo
{
	APawn* Pointer;
	bool bIsVisible;
};

std::vector<PlayerInfo> CachedPlayers{};

HRESULT(*PresentOriginal)(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags) = nullptr;
HRESULT PresentHook(IDXGISwapChain* SwapChain, UINT syncInterval, UINT flags)
{
	if (bIsResizing)
		return PresentOriginal(SwapChain, syncInterval, flags);

	if (!renderTargetView)
	{
		g_pSwapChain = SwapChain;

		SwapChain->GetDevice(__uuidof(device), reinterpret_cast<PVOID*>(&device));
		device->GetImmediateContext(&immediateContext);

		ID3D11Texture2D* renderTarget = nullptr;
		SwapChain->GetBuffer(0, __uuidof(renderTarget), reinterpret_cast<PVOID*>(&renderTarget));
		device->CreateRenderTargetView(renderTarget, nullptr, &renderTargetView);
		renderTarget->Release();

		DXGI_SWAP_CHAIN_DESC desc{};
		SwapChain->GetDesc(&desc);

		hWnd = desc.OutputWindow;

		WndProcOriginal = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProcHook)));

		ImGui_ImplDX11_Init(hWnd, device, immediateContext);
		ImGui_ImplDX11_CreateDeviceObjects();

		// Setup IMGUI color set upon initialization
		{
			auto& style = ImGui::GetStyle();

			// make title text center aligned
			style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

			ImVec4 BlackColor = ImVec4(0.0f, 0.0f, 0.0f, 1.0f), PurpleColor = ImVec4(0.56862745f, 0.101960f, 0.835294f, 1.0f);

			style.Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.0f);
			style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.0f);

			style.Colors[ImGuiCol_WindowBg] = BlackColor;
			style.Colors[ImGuiCol_ChildWindowBg] = BlackColor;
			style.Colors[ImGuiCol_PopupBg] = BlackColor;

			style.Colors[ImGuiCol_Border] = PurpleColor;

			style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.0f);
			style.Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
			style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.0f);
			style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.0f);

			style.Colors[ImGuiCol_TitleBg] = BlackColor;
			style.Colors[ImGuiCol_TitleBgActive] = BlackColor;

			style.Colors[ImGuiCol_MenuBarBg] = BlackColor;

			style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.0f);
			style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
			style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.0f);
			style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.0f);

			style.Colors[ImGuiCol_CheckMark] = PurpleColor;

			style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
			style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.0f);

			style.Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.0f);

			style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.0f);
			style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.0f);
			style.Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.0f);
			style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.0f);
			style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.0f);
			style.Colors[ImGuiCol_Column] = ImVec4(0.56f, 0.56f, 0.58f, 1.0f);
			style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.0f);
			style.Colors[ImGuiCol_ColumnActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.0f);
			style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.0f);
			style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.0f);
			style.Colors[ImGuiCol_CloseButton] = ImVec4(0.40f, 0.39f, 0.38f, 0.16f);
			style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.40f, 0.39f, 0.38f, 0.39f);
			style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.40f, 0.39f, 0.38f, 1.0f);
			style.Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
			style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.0f);
			style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
			style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.0f);
			style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
			style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);

			style.GrabRounding = style.FrameRounding = 2.3f;
		}
	}

	immediateContext->OMSetRenderTargets(1, &renderTargetView, nullptr);

	auto& window = BeginScene();

	if (hWnd != nullptr)
	{
		RECT rect{};
		if (GetWindowRect(hWnd, &rect))
		{
			width = rect.right - rect.left;
			height = rect.bottom - rect.top;
		}
	}

	do
	{
		ClosestEnemy = nullptr;
		if (CachedPlayers.empty() || LocalPlayerController == nullptr || LocalPawn == nullptr)
			break;

		auto LocalWorldLocation = LocalPawn->GetLocation();
		if (LocalWorldLocation == FVector()) break;

		auto ClosestEnemyDistance = FLT_MAX;

		auto TempPlayers = CachedPlayers;
		for (auto Player : TempPlayers)
		{
			auto CurrentPawn = Player.Pointer;
			if (CurrentPawn == LocalPawn || CurrentPawn == nullptr) continue;

			auto CurWorldLoc = CurrentPawn->GetLocation();
			if (CurWorldLoc == FVector()) continue;

			auto DistanceFromLocalPawn = CurWorldLoc.Distance(LocalWorldLocation) / 100.0f;

			auto CurScreenLoc = Engine::ProjectWorldToScreen(CurWorldLoc);
			if (CurScreenLoc == FVector() || !Util::IsLocationInScreen(CurScreenLoc)) continue;

			auto PlayerPawn = static_cast<ATgPawn*>(CurrentPawn);
			if (PlayerPawn == nullptr) continue;

			auto Health = PlayerPawn->GetHealth();
			if (Health <= 0.0f) continue;

			if (PlayerPawn->IsOfSameTeam(LocalPawn)) continue;

			auto CurMesh = CurrentPawn->GetMesh();
			if (CurMesh == nullptr) continue;

			auto CurBounds = CurMesh->GetBounds();

			auto Origin = CurBounds.Origin,
				BoxExtent = CurBounds.BoxExtent;

			FBox CurBox;
			CurBox.Min = Origin - BoxExtent;
			CurBox.Max = Origin + BoxExtent;
			CurBox.IsValid = true;

			auto RootWorldLocation = CurMesh->GetBoneLocation(Player::Bones::Root_bn),
				HeadWorldLocation = CurMesh->GetBoneLocation(Player::Bones::C_Head_BN);
			if (RootWorldLocation == FVector() || HeadWorldLocation == FVector()) continue;

			auto RootScreenLocation = Engine::ProjectWorldToScreen(RootWorldLocation),
				HeadScreenLocation = Engine::ProjectWorldToScreen(HeadWorldLocation);
			if (RootScreenLocation == FVector() || HeadScreenLocation == FVector()) continue;

			auto bIsVisible = Player.bIsVisible;

			auto DrawColor = ImColor(Settings::PlayerVisibleColor[0], Settings::PlayerVisibleColor[1], Settings::PlayerVisibleColor[2], 1.f);
			if (!bIsVisible)
				DrawColor = ImColor(Settings::PlayerInvisibleColor[0], Settings::PlayerInvisibleColor[1], Settings::PlayerInvisibleColor[2], 1.f);

			auto bShouldTarget = true, bShouldDraw = true;
			if (!bIsVisible && Settings::bTargetVischeck)
				bShouldTarget = false;

			auto BoxHeight = ((HeadScreenLocation.Y - RootScreenLocation.Y) * 1.1f);
			auto BoxWidth = (BoxHeight / 2.0f);

			auto BoxPositionX = HeadScreenLocation.X - BoxWidth * 0.5f, BoxPositionY = RootScreenLocation.Y;

			// Do skeleton...
			if (Settings::bPlayerSkeleton)
			{
				auto NeckBoneScreenPosition = Engine::ProjectWorldToScreen(CurMesh->GetBoneLocation(Player::Bones::C_Neck01_BN)),

					LeftShoulderBoneScreenLocation = Engine::ProjectWorldToScreen(CurMesh->GetBoneLocation(Player::Bones::L_WingShoulder01_BN)),
					RightShoulderBoneScreenLocation = Engine::ProjectWorldToScreen(CurMesh->GetBoneLocation(Player::Bones::R_WingShoulder01_BN)),

					LowLeftArmBoneScreenLocation = Engine::ProjectWorldToScreen(CurMesh->GetBoneLocation(Player::Bones::L_Forearm_BN)),
					LowRightArmBoneScreenLocation = Engine::ProjectWorldToScreen(CurMesh->GetBoneLocation(Player::Bones::R_Forearm_BN)),

					LeftElbowBoneScreenLocation = Engine::ProjectWorldToScreen(CurMesh->GetBoneLocation(Player::Bones::L_WingElbow01_BN)),
					RightElbowBoneScreenLocation = Engine::ProjectWorldToScreen(CurMesh->GetBoneLocation(Player::Bones::R_WingElbow01_BN)),

					TopLeftHandBoneScreenLocation = Engine::ProjectWorldToScreen(CurMesh->GetBoneLocation(Player::Bones::L_Hand_BN)),
					TopRightHandBoneScreenLocation = Engine::ProjectWorldToScreen(CurMesh->GetBoneLocation(Player::Bones::R_Hand_BN)),

					LowChestBoneScreenLocation = Engine::ProjectWorldToScreen(CurMesh->GetBoneLocation(Player::Bones::C_Pelvis_BN)),

					LeftLegScreenLocation = Engine::ProjectWorldToScreen(CurMesh->GetBoneLocation(Player::Bones::L_Calf_BN)),
					RightLegScreenLocation = Engine::ProjectWorldToScreen(CurMesh->GetBoneLocation(Player::Bones::R_Calf_BN)),

					LeftThighBoneScreenLocation = Engine::ProjectWorldToScreen(CurMesh->GetBoneLocation(Player::Bones::L_Foot_bn)),
					RightThighBoneScreenLocation = Engine::ProjectWorldToScreen(CurMesh->GetBoneLocation(Player::Bones::R_Foot_bn));


				window.DrawList->AddLine(ImVec2(NeckBoneScreenPosition.X, NeckBoneScreenPosition.Y), ImVec2(HeadScreenLocation.X, HeadScreenLocation.Y), DrawColor, 2.0f);

				window.DrawList->AddLine(ImVec2(LeftShoulderBoneScreenLocation.X, LeftShoulderBoneScreenLocation.Y), ImVec2(NeckBoneScreenPosition.X, NeckBoneScreenPosition.Y), DrawColor, 2.0f);
				window.DrawList->AddLine(ImVec2(LeftElbowBoneScreenLocation.X, LeftElbowBoneScreenLocation.Y), ImVec2(LeftShoulderBoneScreenLocation.X, LeftShoulderBoneScreenLocation.Y), DrawColor, 2.0f);
				window.DrawList->AddLine(ImVec2(LowLeftArmBoneScreenLocation.X, LowLeftArmBoneScreenLocation.Y), ImVec2(LeftElbowBoneScreenLocation.X, LeftElbowBoneScreenLocation.Y), DrawColor, 2.0f);
				window.DrawList->AddLine(ImVec2(TopLeftHandBoneScreenLocation.X, TopLeftHandBoneScreenLocation.Y), ImVec2(LowLeftArmBoneScreenLocation.X, LowLeftArmBoneScreenLocation.Y), DrawColor, 2.0f);

				window.DrawList->AddLine(ImVec2(RightShoulderBoneScreenLocation.X, RightShoulderBoneScreenLocation.Y), ImVec2(NeckBoneScreenPosition.X, NeckBoneScreenPosition.Y), DrawColor, 2.0f);
				window.DrawList->AddLine(ImVec2(RightElbowBoneScreenLocation.X, RightElbowBoneScreenLocation.Y), ImVec2(RightShoulderBoneScreenLocation.X, RightShoulderBoneScreenLocation.Y), DrawColor, 2.0f);
				window.DrawList->AddLine(ImVec2(LowRightArmBoneScreenLocation.X, LowRightArmBoneScreenLocation.Y), ImVec2(RightElbowBoneScreenLocation.X, RightElbowBoneScreenLocation.Y), DrawColor, 2.0f);
				window.DrawList->AddLine(ImVec2(TopRightHandBoneScreenLocation.X, TopRightHandBoneScreenLocation.Y), ImVec2(LowRightArmBoneScreenLocation.X, LowRightArmBoneScreenLocation.Y), DrawColor, 2.0f);

				window.DrawList->AddLine(ImVec2(LowChestBoneScreenLocation.X, LowChestBoneScreenLocation.Y), ImVec2(NeckBoneScreenPosition.X, NeckBoneScreenPosition.Y), DrawColor, 2.0f);

				window.DrawList->AddLine(ImVec2(LeftLegScreenLocation.X, LeftLegScreenLocation.Y), ImVec2(LowChestBoneScreenLocation.X, LowChestBoneScreenLocation.Y), DrawColor, 2.0f);
				window.DrawList->AddLine(ImVec2(RightLegScreenLocation.X, RightLegScreenLocation.Y), ImVec2(LowChestBoneScreenLocation.X, LowChestBoneScreenLocation.Y), DrawColor, 2.0f);

				window.DrawList->AddLine(ImVec2(LeftThighBoneScreenLocation.X, LeftThighBoneScreenLocation.Y), ImVec2(LeftLegScreenLocation.X, LeftLegScreenLocation.Y), DrawColor, 2.0f);
				window.DrawList->AddLine(ImVec2(RightThighBoneScreenLocation.X, RightThighBoneScreenLocation.Y), ImVec2(RightLegScreenLocation.X, RightLegScreenLocation.Y), DrawColor, 2.0f);
			}

			if (Settings::bPlayerHead)
			{
				auto HeadCircleRadius = 0.0f;

				// We want to adjust how large the circle is based on the distance the player is from us as to make sure it does not look odd.
				if (DistanceFromLocalPawn >= 100.0f) HeadCircleRadius = 0.5f;
				else if (DistanceFromLocalPawn >= 50.0f) HeadCircleRadius = 1.0f;
				else if (DistanceFromLocalPawn >= 20.0f) HeadCircleRadius = 3.0f;
				else HeadCircleRadius = 10.0f;

				window.DrawList->AddCircle(ImVec2(HeadScreenLocation.X, HeadScreenLocation.Y), HeadCircleRadius, DrawColor, 100, 2.0f);
			}

			if (Settings::bPlayerBox)
			{
				switch (SelectedBoxStyle)
				{
				case 0:
					window.DrawList->AddRect(ImVec2(BoxPositionX, BoxPositionY), ImVec2(BoxPositionX + BoxWidth, BoxPositionY + BoxHeight), DrawColor);
					break;
				case 1:
					window.DrawList->AddRectFilled(ImVec2(BoxPositionX, BoxPositionY), ImVec2(BoxPositionX + BoxWidth, BoxPositionY + BoxHeight), ImGui::GetColorU32({ 0.f, 0.f, 0.f, 0.4f }));
					window.DrawList->AddRect(ImVec2(BoxPositionX, BoxPositionY), ImVec2(BoxPositionX + BoxWidth, BoxPositionY + BoxHeight), DrawColor);
					break;
				case 2:
					Util::DrawCorneredBox(window, BoxPositionX, BoxPositionY, BoxWidth, BoxHeight, DrawColor, DrawColor);
					break;
				case 3:
					window.DrawList->AddRectFilled(ImVec2(BoxPositionX, BoxPositionY), ImVec2(BoxPositionX + BoxWidth, BoxPositionY + BoxHeight), ImGui::GetColorU32({ 0.f, 0.f, 0.f, 0.4f }));
					Util::DrawCorneredBox(window, BoxPositionX, BoxPositionY, BoxWidth, BoxHeight, DrawColor, DrawColor);
					break;
				case 4:
					Engine::Boxes(window, LocalPlayerController, DrawColor, CurBox);
					break;
				}
			}

			if (Settings::bPlayerLine)
			{
				window.DrawList->AddLine(ImVec2(width / 2, height / 2), ImVec2(HeadScreenLocation.X, HeadScreenLocation.Y), DrawColor);
			}

			if (Settings::bPlayerDistance || Settings::bPlayerName)
			{
				std::string MessageToRender{};

				if (Settings::bPlayerDistance)
					MessageToRender = (std::string("[") + std::to_string(static_cast<int>(DistanceFromLocalPawn)) + std::string("m]"));

				auto Message = MessageToRender.c_str();
				auto TextSize = ImGui::GetFont()->CalcTextSizeA(window.DrawList->_Data->FontSize, FLT_MAX, 0, Message);

				auto StartX = (BoxPositionX + BoxWidth) - (TextSize.x * 0.5f),
					StartY = ((BoxPositionY + BoxHeight) - TextSize.y);

				window.DrawList->AddRectFilled(
					ImVec2(StartX, StartY),
					ImVec2(StartX + TextSize.x, StartY + TextSize.y),
					IM_COL32(0, 0, 0, 255));

				window.DrawList->AddText(ImVec2(StartX, StartY), DrawColor, Message);
			}

			if (bShouldTarget)
			{
				// Get targeting data...
				auto ScreenLocationX = HeadScreenLocation.X - (width / 2.0f),
					ScreenLocationY = HeadScreenLocation.Y - (height / 2.0f);

				auto ActorDistance = sqrtf(ScreenLocationX * ScreenLocationX + ScreenLocationY * ScreenLocationY);
				if (ActorDistance < ClosestEnemyDistance && ActorDistance < Settings::TargetingFOV)
				{
					ClosestEnemy = PlayerPawn;
					ClosestEnemyDistance = ActorDistance;
				}
			}
		}

		// Aimbot options... Silent and memory...
		if (Settings::bAimbot)
		{
			auto HitBoxTarget = FVector();
			if (ClosestEnemy != nullptr)
			{
				auto Mesh = ClosestEnemy->GetMesh();
				if (Mesh == nullptr) break;

				if (!strcmp(SelectedHitbox, "HEAD"))
					HitBoxTarget = Mesh->GetBoneLocation(Player::Bones::C_Head_BN);
				else if (!strcmp(SelectedHitbox, "NECK"))
					HitBoxTarget = Mesh->GetBoneLocation(Player::Bones::C_Neck01_BN);
				else if (!strcmp(SelectedHitbox, "CHEST"))
					HitBoxTarget = Mesh->GetBoneLocation(Player::Bones::C_Pelvis_BN);
			}

			if (!strcmp(SelectedAimbot, "SILENT"))
			{
				// Fake Silent aimbot...
				static auto OldAimRotation = FRotator();
				if (bIsShooting && ClosestEnemy && LocalPawn)
				{
					auto CameraLocation = ACameraCache::GetLocation();
					if (HitBoxTarget == FVector() || CameraLocation == FVector()) break;

					auto AimAngle = Engine::AimAtVector(HitBoxTarget, CameraLocation);

					if (AimAngle != FRotator())
					{
						OldRotation = ACameraCache::GetRotation();
						if (OldAimRotation == FRotator())
						{
							OldAimRotation = read<FRotator>(LocalPlayerController->GetReference() + 0x8c);
							if (OldAimRotation == FRotator()) break;
						}

						if (bLockedCamera)
							write<FRotator>(LocalPlayerController->GetReference() + 0x8c, AimAngle);
					}
				}
				else if (OldAimRotation != FRotator())
				{
					write<FRotator>(LocalPlayerController->GetReference() + 0x8c, OldAimRotation);
					OldAimRotation = FRotator(), OldRotation = FRotator();
				}
			}
			else if (!strcmp(SelectedAimbot, "MEMORY") && GetAsyncKeyState(Settings::TargetingKey))
			{
				if (ClosestEnemy && LocalPawn)
				{
					auto CameraLocation = ACameraCache::GetLocation();
					if (HitBoxTarget == FVector() || CameraLocation == FVector()) break;

					auto AimAngle = Engine::AimAtVector(HitBoxTarget, CameraLocation);
					if (Settings::TargetingSpeed > 0)
					{
						auto CurrentAngles = read<FRotator>(LocalPlayerController->GetReference() + 0x8c);
						if (CurrentAngles != FRotator() && WorldInfo != nullptr)
						{	
							AimAngle = UObject::RInterpTo(CurrentAngles, AimAngle,
								WorldInfo->GetDeltaSeconds(), Settings::TargetingSpeed, false);
						}
					}

					if (AimAngle != FRotator())
						write<FRotator>(LocalPlayerController->GetReference() + 0x8c, AimAngle);
				}
			}
		}

		// Set weapon values...
		auto CurrentWeapon = LocalPawn->GetWeapon();
		if (CurrentWeapon.data != NULL)
		{
			if (Settings::bNoSpread)
			{
				auto accuracy = read<FAccuracySettings>(CurrentWeapon.data + 0x944);
				accuracy.fAccuracyGainPerSec = 0;
				accuracy.fMaxAccuracy = 1;
				accuracy.fMinAccuracy = 1;
				write(CurrentWeapon.data + 0x944, accuracy);
				write(CurrentWeapon.data + 0x944 + sizeof(FAccuracySettings), accuracy);
			}

			if (Settings::bNoRecoil)
			{
				auto recoil = read<FRecoilSettings>(CurrentWeapon.data + 0x9E4);
				recoil.bUsesRecoil = 1;
				recoil.fRecoilReductionPerSec = 0;
				recoil.fRecoilCenterDelay = 0;
				recoil.fRecoilSmoothRate = 0;
				write(CurrentWeapon.data + 0x9E4, recoil);
				write(CurrentWeapon.data + 0x9E4 + sizeof(FRecoilSettings), recoil);
			}
		}

		if (Settings::bInstantMount && LocalPawn != nullptr)
			LocalPawn->SetInstantMount();
	} while (false);

	if (Settings::bDrawFOV)
	{
		window.DrawList->AddCircle(ImVec2(width / 2.0f, height / 2.0f), Settings::TargetingFOV, ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, 1.0f }), 100, 2.0f);
	}

	if (Settings::bDrawCrosshair)
	{
		window.DrawList->AddLine(ImVec2((width / 2.0f) + 4.0f, (height / 2.0f)),
			ImVec2((width / 2.0f) + 12.0f, (height / 2.0f)), ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, 1.0f }), 2.0f);
		window.DrawList->AddLine(ImVec2((width / 2.0f) - 4.0f, (height / 2.0f)),
			ImVec2((width / 2.0f) - 12.0f, (height / 2.0f)), ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, 1.0f }), 2.0f);

		window.DrawList->AddLine(ImVec2((width / 2.0f), (height / 2.0f) + 4.0f),
			ImVec2((width / 2.0f), (height / 2.0f) + 12.0f), ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, 1.0f }), 2.0f);
		window.DrawList->AddLine(ImVec2((width / 2.0f), (height / 2.0f) - 4.0f),
			ImVec2((width / 2.0f), (height / 2.0f) - 12.0f), ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, 1.0f }), 2.0f);
	}

	EndScene(window);
	return PresentOriginal(SwapChain, syncInterval, flags);
}

auto SetRenderHook() -> bool
{
	IDXGISwapChain* swapChain = nullptr;
	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* context = nullptr;
	auto                 featureLevel = D3D_FEATURE_LEVEL_11_0;

	DXGI_SWAP_CHAIN_DESC sd = { 0 };
	sd.BufferCount = 1;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.OutputWindow = GetForegroundWindow();
	sd.SampleDesc.Count = 1;
	sd.Windowed = TRUE;

	if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, 0, &featureLevel,
		1, D3D11_SDK_VERSION, &sd, &swapChain, &device, nullptr, &context)))
	{
		return false;
	}

	auto table = *reinterpret_cast<PVOID**>(swapChain);
	auto present = table[8];
	auto resize = table[13];

	context->Release();
	device->Release();
	swapChain->Release();

	MH_Initialize();

	MH_CreateHook(present, PresentHook, reinterpret_cast<PVOID*>(&PresentOriginal));
	MH_EnableHook(present);

	return true;
}

// These hooks allow us to intercept when the character starts and stops shooting, from there we can manipulate values to "silently" aimbot...
__int64(*DeviceOnStartFireOriginal)(APawn* Pawn, uintptr_t Device) = nullptr;
__int64 DeviceOnStartFireHook(APawn* Pawn, uintptr_t Device)
{
	bIsShooting = true;
	return DeviceOnStartFireOriginal(Pawn, Device);
}

__int64(*DeviceOnStopFireOriginal)(APawn* Pawn, uintptr_t Device, bool bWasInterrupted) = nullptr;
__int64 DeviceOnStopFireHook(APawn* Pawn, uintptr_t Device, bool bWasInterrupted)
{
	bIsShooting = false;
	return DeviceOnStopFireOriginal(Pawn, Device, bWasInterrupted);
}

// We are doing our operations from a game thread as calling StaticFindObject outside of one can cause instability...
void ProcessEventHook(UObject* ClassObject, UObject* FunctionObject, uintptr_t Parameters, uintptr_t Return)
{
	do
	{
		auto EngineContext = UEngine::GetEngine();
		if (!EngineContext) break;

		auto GamePlayers = EngineContext->GetGamePlayers();
		if (!GamePlayers) break;

		auto LocalPlayer = read<ULocalPlayer*>(GamePlayers);
		if (!LocalPlayer) break;

		LocalPlayerController = LocalPlayer->GetPlayerController();
		if (!LocalPlayerController) break;

		LocalPawn = LocalPlayerController->GetPawn();
		if (LocalPawn == nullptr) break;

		// Filter for our own PlayerController events in order to limit performance hindering...
		if (LocalPlayerController == ClassObject)
		{
			auto LocalPawnVFTable = read<uintptr_t*>(LocalPawn->GetReference());
			if (LocalPawnVFTable != nullptr)
			{
				static std::unique_ptr<uintptr_t[]> NewLocalPawnVFTable = nullptr;

				if (!NewLocalPawnVFTable.get())
				{
					auto VFunctionRealCount = Util::GetVFunctionCount(LocalPawnVFTable);
					NewLocalPawnVFTable = std::make_unique<uintptr_t[]>(++VFunctionRealCount);
				}

				if (NewLocalPawnVFTable.get() != LocalPawnVFTable)
				{
					// DeviceOnStartFire: (*(__int64 (__fastcall **)(__int64, __int64))(*(_QWORD *)Pawn + 0x1618i64))(Pawn, Dev);
					// DeviceOnStopFire: (*(__int64 (__fastcall **)(__int64, __int64, _QWORD))(*(_QWORD *)v2 + 0x1620i64))(Pawn, Dev, (unsigned int)WasInterrupted);
					Util::ShadowVMTHookFunction(LocalPawn->GetReference(), NewLocalPawnVFTable.get(),
						{ reinterpret_cast<uintptr_t>(&DeviceOnStartFireHook), reinterpret_cast<uintptr_t>(&DeviceOnStopFireHook) },
						{ reinterpret_cast<uintptr_t*>(&DeviceOnStartFireOriginal), reinterpret_cast<uintptr_t*>(&DeviceOnStopFireOriginal) },
						{ 707, 708 }, 0);
				}
			}

			auto LocalPlayerCamera = LocalPlayerController->GetPlayerCamera();
			if (LocalPlayerCamera == nullptr) break;

			WorldInfo = AWorldInfo::GetWorldInfo();
			if (WorldInfo == nullptr) break;

			static auto
				OriginalTimeDilation = 0.0f,
				OriginalWorldDilation = 0.0f,
				OriginalDemoPlayDilation = 0.0f;
			if (Settings::bLocalSpeedhack && GetAsyncKeyState(VK_F2))
			{
				if (OriginalTimeDilation == 0.0f)
					OriginalTimeDilation = LocalPawn->GetCustomTimeDilation();

				if (OriginalWorldDilation == 0.0f)
					OriginalWorldDilation = read<float>(WorldInfo->GetReference() + 0x4E4);

				if (OriginalDemoPlayDilation == 0.0f)
					OriginalDemoPlayDilation = read<float>(WorldInfo->GetReference() + 0x4E8);

				if (LocalPawn != nullptr)
					LocalPawn->SetCustomTimeDilation(Settings::SpeedHackValue);

				write<float>(WorldInfo->GetReference() + 0x4E4, Settings::SpeedHackValue); // WorldInfo->TimeDilation
				write<float>(WorldInfo->GetReference() + 0x4E8, Settings::SpeedHackValue); // WorldInfo->DemoPlayTimeDilation
			}
			else if (OriginalTimeDilation || OriginalWorldDilation || OriginalDemoPlayDilation)
			{
				if (LocalPawn != nullptr)
					LocalPawn->SetCustomTimeDilation(OriginalTimeDilation);

				write<float>(WorldInfo->GetReference() + 0x4E4, OriginalWorldDilation); // WorldInfo->TimeDilation
				write<float>(WorldInfo->GetReference() + 0x4E8, OriginalDemoPlayDilation); // WorldInfo->DemoPlayTimeDilation

				OriginalTimeDilation = OriginalWorldDilation = OriginalDemoPlayDilation = 0.0f;
			}

			auto PlayerCameraVFTable = read<uintptr_t*>(LocalPlayerCamera->GetReference());
			if (PlayerCameraVFTable != nullptr)
			{
				static std::unique_ptr<uintptr_t[]> NewCameraVFTable = nullptr;
				if (!NewCameraVFTable.get())
				{
					auto VFunctionRealCount = Util::GetVFunctionCount(PlayerCameraVFTable);
					NewCameraVFTable = std::make_unique<uintptr_t[]>(++VFunctionRealCount);
				}

				if (NewCameraVFTable.get() != PlayerCameraVFTable)
				{
					Util::ShadowVMTHookFunction(LocalPlayerCamera->GetReference(), NewCameraVFTable.get(),
						{ reinterpret_cast<uintptr_t>(&FillCameraCacheHook) },
						{ reinterpret_cast<uintptr_t*>(&FillCameraCacheOriginal) },
						{ 272 }, 0);
				}
			}

			// Iterate all player actors...
			auto CurrentPawn = WorldInfo->GetPawnList();
			std::vector<PlayerInfo> TempPlayers{};

			while (CurrentPawn)
			{
				if (CurrentPawn == nullptr) break;

				if (LocalPawn != nullptr && CurrentPawn == LocalPawn)
				{
					CurrentPawn = CurrentPawn->GetNextPawn();
					continue;
				}

				auto CurMesh = CurrentPawn->GetMesh();
				if (CurMesh == nullptr)
				{
					CurrentPawn = CurrentPawn->GetNextPawn();
					continue;
				}

				PlayerInfo Info{};
				Info.bIsVisible = CurMesh->IsVisible(WorldInfo->GetTimeSeconds());
				Info.Pointer = CurrentPawn;

				TempPlayers.push_back(Info);
				CurrentPawn = CurrentPawn->GetNextPawn();
			}

			CachedPlayers = TempPlayers;
		}
	} while (false);

	ProcessEventOriginal(ClassObject, FunctionObject, Parameters, Return);
}

auto SetGameHook() -> bool
{
	// [actual address in first opcode] E8 ? ? ? ? 8D 4E FD
	auto addr = Util::PatternScan(
		Settings::GameBaseAddress,
		Settings::GameSizeOfImage,
		L"\xE8\x00\x00\x00\x00\x8D\x4E\xFD",
		L"x????xxx"
	);

	if (addr == NULL)
	{
		MessageBoxA(nullptr, "ProcessEvent reference not found!", "OK", MB_OK);
		return false;
	}

	addr = RVA(addr, 5);

	if (MH_Initialize() != MH_OK)
	{
		MessageBoxA(nullptr, "Failed to initialize Minhook!", "Failure", MB_ICONERROR);

		return false;
	}

	if (MH_CreateHook((LPVOID)addr, ProcessEventHook, (LPVOID*)&ProcessEventOriginal) != MH_OK)
	{
		MessageBoxA(nullptr, "Failed to create hook on ProcessEvent!", "Failure", MB_ICONERROR);
		return false;
	}

	if (MH_EnableHook((LPVOID)addr) != MH_OK)
	{
		MessageBoxA(nullptr, "Failed to enable hook on ProcessEvent!", "Failure", MB_ICONERROR);
		return false;
	}

	return true;
}

bool Initialize()
{
	Settings::GameBaseAddress = reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr));
	if (Settings::GameBaseAddress == NULL) return false;

	Settings::GameSizeOfImage = reinterpret_cast<PIMAGE_NT_HEADERS>(
		reinterpret_cast<std::uint8_t*>(Settings::GameBaseAddress) +
		reinterpret_cast<PIMAGE_DOS_HEADER>(Settings::GameBaseAddress)->e_lfanew)->OptionalHeader.SizeOfImage;
	if (Settings::GameSizeOfImage == NULL) return false;

	return (SetGameHook() && SetRenderHook());
}

bool DllMain(void** hinstDLL, unsigned long fdwReason, void* lpReserved)
{
	return Initialize();
}