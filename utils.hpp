#pragma once

class UObject;
class AActor;
class APlayerController;

namespace ACameraCache
{
	FVector CameraLocation;
	FRotator CameraRotation;
	float CameraFOVAngle;

	// Setting info, can be used for updating, etc.
	auto SetLocation(FVector NewCameraLocation) -> void
	{
		CameraLocation = NewCameraLocation;
	}

	auto SetRotation(FRotator NewCameraRotation) -> void
	{
		CameraRotation = NewCameraRotation;
	}

	auto SetFOVAngle(float NewCameraFOVAngle) -> void
	{
		CameraFOVAngle = NewCameraFOVAngle;
	}

	// Getting the info
	auto GetLocation() -> FVector
	{
		return CameraLocation;
	}

	auto GetRotation() -> FRotator
	{
		return CameraRotation;
	}

	auto GetFOVAngle() -> float
	{
		return CameraFOVAngle;
	}
}

// General cheat based functionality
namespace Util
{
	auto PatternScan(uintptr_t base, DWORD size,
		const wchar_t* pattern, const wchar_t* mask) -> uintptr_t
	{
		auto MaskCompare = [](void* buffer, const wchar_t* pattern, const wchar_t* mask)
		{
			for (auto b = reinterpret_cast<PBYTE>(buffer); *mask; ++pattern, ++mask, ++b)
			{
				if (*mask == 'x' && *reinterpret_cast<const wchar_t*>(pattern) != *b)
					return false;
			}

			return true;
		};

		size -= static_cast<DWORD>(wcslen(mask));

		for (auto i = 0UL; i < size; ++i)
		{
			auto addr = reinterpret_cast<PBYTE>(base) + i;
			if (MaskCompare(addr, pattern, mask))
				return reinterpret_cast<uintptr_t>(addr);
		}

		return 0;
	}

	auto GetVFTable(uintptr_t Object, uint32_t Offset) -> uintptr_t
	{
		return read<uintptr_t>(Object + Offset);
	}

	// this will return the array index based count (which is 0 based).
	auto GetVFunctionCount(uintptr_t* VFTable) -> int
	{
		auto TotalVFunctions = 0;
		while (VFTable[TotalVFunctions]) TotalVFunctions++;

		return TotalVFunctions;
	}

	auto ShadowVMTHookFunction(
		uintptr_t ClassPointer,
		uintptr_t* NewVFTable,
		std::vector<uintptr_t> NewFunctions,
		std::vector<uintptr_t*> OriginalFunctions,
		std::vector<uint32_t> FunctionIndexes,
		uint32_t VFTableOffset)
	{
		auto VFTable = read<uintptr_t*>(ClassPointer + VFTableOffset);
		if (VFTable == nullptr) return;

		auto TotalVFunctions = GetVFunctionCount(VFTable);
		if (TotalVFunctions == NULL) return;

		for (auto FunctionIndex = 0; FunctionIndex < TotalVFunctions; ++FunctionIndex)
			NewVFTable[FunctionIndex] = VFTable[FunctionIndex];

		for (auto CurrentFunctionIndex = 0; CurrentFunctionIndex < FunctionIndexes.size(); ++CurrentFunctionIndex)
		{
			auto NewFunctionAddress = NewFunctions.at(CurrentFunctionIndex);
			auto OriginalFunctionAddress = OriginalFunctions.at(CurrentFunctionIndex);
			auto CurrentFunctionIndexToReplace = FunctionIndexes.at(CurrentFunctionIndex);

			*OriginalFunctionAddress = VFTable[CurrentFunctionIndexToReplace];
			NewVFTable[CurrentFunctionIndexToReplace] = NewFunctionAddress;
		}

		*reinterpret_cast<uintptr_t**>(ClassPointer + VFTableOffset) = NewVFTable;
	}

	inline bool IsLocationInScreen(FVector ScreenLocation, int over = 10)
	{
		if (ScreenLocation.X > -over && ScreenLocation.X < width + over &&
			ScreenLocation.Y > -over && ScreenLocation.Y < height + over)
			return true;

		return false;
	}

	auto CreateConsole() -> VOID
	{
		AllocConsole();

		static_cast<void>(freopen("conin$", "r", stdin));
		static_cast<void>(freopen("conout$", "w", stdout));
		static_cast<void>(freopen("conout$", "w", stderr));
	}

	inline auto DrawCorneredBox(ImGuiWindow& window, FLOAT X, FLOAT Y,
		FLOAT W, FLOAT H, ImU32 InlineColor, ImU32 OutLineColor) -> VOID
	{
		auto lineW = (W / 5.0f), lineH = (H / 6.0f), lineT = 1.0f;

		// Inline
		window.DrawList->AddLine(ImVec2(X, Y), ImVec2(X, Y + lineH), InlineColor, 1.0f);
		window.DrawList->AddLine(ImVec2(X, Y), ImVec2(X + lineW, Y), InlineColor, 1.0f);
		window.DrawList->AddLine(ImVec2(X + W - lineW, Y), ImVec2(X + W, Y), InlineColor, 1.0f);
		window.DrawList->AddLine(ImVec2(X + W, Y), ImVec2(X + W, Y + lineH), InlineColor, 1.0f);
		window.DrawList->AddLine(ImVec2(X, Y + H - lineH), ImVec2(X, Y + H), InlineColor, 1.0f);
		window.DrawList->AddLine(ImVec2(X, Y + H), ImVec2(X + lineW, Y + H), InlineColor, 1.0f);
		window.DrawList->AddLine(ImVec2(X + W - lineW, Y + H), ImVec2(X + W, Y + H), InlineColor, 1.0f);
		window.DrawList->AddLine(ImVec2(X + W, Y + H - lineH), ImVec2(X + W, Y + H), InlineColor, 1.0f);

		// Outline
		window.DrawList->AddLine(ImVec2(X - lineT, Y - lineT), ImVec2(X + lineW, Y - lineT), OutLineColor, 1.0f);
		window.DrawList->AddLine(ImVec2(X - lineT, Y - lineT), ImVec2(X - lineT, Y + lineH), OutLineColor, 1.0f);
		window.DrawList->AddLine(ImVec2(X - lineT, Y + H - lineH), ImVec2(X - lineT, Y + H + lineT), OutLineColor, 1.0f);
		window.DrawList->AddLine(ImVec2(X - lineT, Y + H + lineT), ImVec2(X + lineW, Y + H + lineT), OutLineColor, 1.0f);
		window.DrawList->AddLine(ImVec2(X + W - lineW, Y - lineT), ImVec2(X + W + lineT, Y - lineT), OutLineColor, 1.0f);
		window.DrawList->AddLine(ImVec2(X + W + lineT, Y - lineT), ImVec2(X + W + lineT, Y + lineH), OutLineColor, 1.0f);
		window.DrawList->AddLine(ImVec2(X + W + lineT, Y + H - lineH), ImVec2(X + W + lineT, Y + H + lineT), OutLineColor, 1.0f);
		window.DrawList->AddLine(ImVec2(X + W - lineW, Y + H + lineT), ImVec2(X + W + lineT, Y + H + lineT), OutLineColor, 1.0f);
	}

	auto Items_ArrayGetter(void* data, int idx, const char** out_text)
	{
		const char* const* items = (const char* const*)data;
		if (out_text)
			*out_text = items[idx];

		return true;
	}

	void GetTargetingButton()
	{
		const char* KeyPreviewName = NULL;
		if (Settings::TargetingKey >= 0 && Settings::TargetingKey < IM_ARRAYSIZE(KeyNames))
			Items_ArrayGetter(KeyNames, Settings::TargetingKey, &KeyPreviewName);

		std::string ButtonMessage{};
		if (Settings::TargetingKey == 0)
			ButtonMessage = "Press key to bind";
		else if (KeyPreviewName)
			ButtonMessage = KeyPreviewName;

		if (ImGui::Button(ButtonMessage.c_str(), ImVec2(125, 20)))
			Settings::TargetingKey = 0;

		if (Settings::TargetingKey == 0)
		{
			for (auto i = 0; i < 0x87; ++i)
			{
				if (GetKeyState(i) & 0x8000)
					Settings::TargetingKey = i;
			}
		}
	}
}

void(*ProcessEventOriginal)(UObject*, UObject*, uintptr_t, uintptr_t);

/* Engine related content below... */
class UObject
{
public:
	auto GetReference()
	{
		return reinterpret_cast<uintptr_t>(this);
	}

	static auto ProcessEvent(UObject* Class, UObject* Function, uintptr_t Params, uintptr_t Result = 0)
	{
		ProcessEventOriginal(Class, Function, Params, Result);
	}

	static auto StaticFindObject(UObject* Class, UObject* InOuter, const wchar_t* Name, bool ExactClass) -> UObject*
	{
		static uintptr_t StaticFindObjectAddress = 0;
		if (!StaticFindObjectAddress)
		{
			// [actual address in first opcode] E8 ? ? ? ? 48 8B F8 EB 6E
			StaticFindObjectAddress = RVA(Util::PatternScan(
				Settings::GameBaseAddress,
				Settings::GameSizeOfImage,
				L"\xE8\x00\x00\x00\x00\x48\x8B\xF8\xEB\x6E",
				L"x????xxxxx"
			), 5);

			if (!StaticFindObjectAddress) return nullptr;
		}

		return reinterpret_cast<UObject * (__fastcall*)(UObject*, UObject*,
			const wchar_t*, bool)>(StaticFindObjectAddress)(Class, InOuter, Name, ExactClass);
	}

	static auto GetStaticClass()
	{
		static UObject* ObjectClass = nullptr;
		if (!ObjectClass)
			ObjectClass = UObject::StaticFindObject(nullptr, nullptr, L"Core.Object", false);

		return ObjectClass;
	}

	static auto RInterpTo(
		const FRotator& Current,
		const FRotator& Target,
		float DeltaTime,
		float InterpSpeed,
		bool bConstantInterpSpeed) -> FRotator
	{
		auto ObjectStaticClass = UObject::GetStaticClass();
		if (ObjectStaticClass == nullptr) return FRotator();

		static UObject* RInterpToObject = nullptr;
		if (RInterpToObject == nullptr)
		{
			RInterpToObject = UObject::StaticFindObject(nullptr, nullptr, L"Core.Object.RInterpTo", false);
			if (RInterpToObject == nullptr) return FRotator();
		}

		struct
		{
			FRotator Current;
			FRotator Target;
			float DeltaTime;
			float InterpSpeed;
			bool bConstantInterpSpeed;
			FRotator ReturnValue;
		} params;

		params.Current = Current;
		params.Target = Target;
		params.DeltaTime = DeltaTime;
		params.InterpSpeed = InterpSpeed;
		params.bConstantInterpSpeed = bConstantInterpSpeed;

		UObject::ProcessEvent(ObjectStaticClass, RInterpToObject, reinterpret_cast<uintptr_t>(&params), 0);
		return params.ReturnValue;
	}
};

class UEngine : public UObject
{
public:
	static auto GetStaticClass()
	{
		static UObject* EngineClass = nullptr;
		if (EngineClass == nullptr)
			EngineClass = UObject::StaticFindObject(nullptr, nullptr, L"Engine.Engine", false);

		return EngineClass;
	}

	static auto GetEngine() -> UEngine*
	{
		auto EngineStaticClass = UEngine::GetStaticClass();
		if (!EngineStaticClass) return nullptr;

		static UObject* GetEngineObject = nullptr;
		if (GetEngineObject == nullptr)
		{
			GetEngineObject = UObject::StaticFindObject(nullptr, nullptr, L"Engine.Engine.GetEngine", false);

			if (GetEngineObject == nullptr) return nullptr;
		}

		struct
		{
			UEngine* ReturnValue;
		} params;

		UObject::ProcessEvent(EngineStaticClass, GetEngineObject, reinterpret_cast<uintptr_t>(&params), 0);
		return params.ReturnValue;
	}

	auto GetGamePlayers()
	{
		return read<uintptr_t>(this->GetReference() + 0x6D4);
	}
};

class ULocalPlayer : public UObject
{
public:
	auto GetPlayerController()
	{
		return read<APlayerController*>(this->GetReference() + 0x68);
	}

	auto GetLastViewLocation()
	{
		return read<FVector>(this->GetReference() + 0xEC);
	}

	auto Project(FVector WorldLocation, FVector& OutLocation) -> void
	{
		static UObject* ProjectFunctionObject = nullptr;
		if (ProjectFunctionObject == nullptr)
		{
			ProjectFunctionObject = UObject::StaticFindObject(nullptr, nullptr, L"Engine.LocalPlayer.Project", false);

			if (ProjectFunctionObject == nullptr) return;
		}

		struct
		{
			FVector WorldLoc;
			FVector ReturnValue;
		} ProjectParam;

		ProjectParam.WorldLoc = WorldLocation;
		ProjectParam.ReturnValue = OutLocation;

		UObject::ProcessEvent(this, ProjectFunctionObject, reinterpret_cast<uintptr_t>(&ProjectParam), 0);
	}
};

class UPrimitiveComponent : public UObject
{
public:
	struct FBoxSphereBounds
	{
		struct FVector                                     Origin;                                        // 0x0000 (0x000C) [0x0000000000000001] (CPF_Edit)    
		struct FVector                                     BoxExtent;                                     // 0x000C (0x000C) [0x0000000000000001] (CPF_Edit)    
		float                                              SphereRadius;                                  // 0x0018 (0x0004) [0x0000000000000001] (CPF_Edit)    
	};

	auto GetBounds()
	{
		return read<FBoxSphereBounds>(this->GetReference() + 0xA0);
	}
};

class UStaticMeshComponent : public UPrimitiveComponent
{
public:
	auto GetStaticMesh()
	{
		return read<uintptr_t>(this->GetReference() + 0x29C);
	}

	auto GetBoneMatrix(int BoneIndex) -> FMatrix
	{
		static uintptr_t GetBoneMatrixAddress = 0;
		if (!GetBoneMatrixAddress)
		{
			// [actual address in first opcode] E8 ? ? ? ? 4D 8D 47 70
			GetBoneMatrixAddress = RVA(Util::PatternScan(
				Settings::GameBaseAddress,
				Settings::GameSizeOfImage,
				L"\xE8\x00\x00\x00\x00\x4D\x8D\x47\x70",
				L"x????xxxx"
			), 5);

			if (!GetBoneMatrixAddress) return FMatrix{};
		}

		FMatrix BoneMatrix{};
		reinterpret_cast<FMatrix * (__fastcall*)(UStaticMeshComponent*, FMatrix*, int)>
			(GetBoneMatrixAddress)(this, &BoneMatrix, BoneIndex);

		return BoneMatrix;
	}

	auto GetBoneLocation(int BoneIndex) -> FVector
	{
		return this->GetBoneMatrix(BoneIndex).WPlane;
	}

	auto GetLastRenderTime() -> float
	{
		return read<float>(this->GetReference() + 0x244);
	}

	auto IsVisible(float timeSeconds) -> bool
	{
		auto lastRenderTime = GetLastRenderTime();
		return (lastRenderTime > timeSeconds - 0.05f);
	}
};

class AActor : public UObject
{
public:
	auto GetLocation()
	{
		return read<FVector>(this->GetReference() + 0x80);
	}

	auto GetRotation()
	{
		return read<FVector>(this->GetReference() + 0x8c);
	}

	auto GetVelocity()
	{
		return read<FVector>(this->GetReference() + 0x190);
	}

	auto GetCustomTimeDilation()
	{
		return read<float>(this->GetReference() + 0xb8);
	}

	auto SetCustomTimeDilation(float NewTimeDilation)
	{
		write<float>(this->GetReference() + 0xb8, NewTimeDilation);
	}

	auto GetComponentsBoundingBox(FBox& ActorBox)
	{
		static UObject* GetComponentsBoundingBoxObject = nullptr;
		if (GetComponentsBoundingBoxObject == nullptr)
		{
			GetComponentsBoundingBoxObject = UObject::StaticFindObject(nullptr, nullptr, L"Engine.Actor.GetComponentsBoundingBox", false);

			if (GetComponentsBoundingBoxObject == nullptr) return false;
		}

		struct
		{
			FBox ReturnValue;
		} params;

		UObject::ProcessEvent(this, GetComponentsBoundingBoxObject, reinterpret_cast<uintptr_t>(&params), 0);
		ActorBox = params.ReturnValue;
	}
};

class ATgRepInfo_TaskForce : public AActor
{
public:
	static auto GetStaticClass() -> UObject*
	{
		static UObject* WorldInfoClass = nullptr;
		if (WorldInfoClass == nullptr)
			WorldInfoClass = UObject::StaticFindObject(nullptr, nullptr, L"TgGame.TgRepInfo_TaskForce", false);

		return WorldInfoClass;
	}

	auto GetnTaskForce()
	{
		return read<int32_t>(this->GetReference() + 0x2D4);
	}
};

class APlayerReplicationInfo : public AActor
{
public:

};

class ATgRepInfo_Player : public APlayerReplicationInfo
{
public:
	auto GetTaskForce()
	{
		return read<ATgRepInfo_TaskForce*>(this->GetReference() + 0x4CC);
	}
};

class ATgDevice
{
public:
	inline int GetAmmoCount()
	{
		return read<int>(data + 0x4CC);
	}

	inline int GetMaxAmmoCount()
	{
		return read<int>(data + 0x4D4);
	}

	uint64_t data;
};

class APawn : public AActor
{
public:
	auto SetInstantMount() -> void
	{
		write<float>(this->GetReference() + 0x2e2c, 0.0f);
	}

	auto GetNextPawn()
	{
		return read<APawn*>(this->GetReference() + 0x2ac);
	}

	auto IsChicken() -> bool
	{
		return static_cast<bool>(read<int>(this->GetReference() + 0x728) == 9204);
	}

	auto GetMesh()
	{
		return read<UStaticMeshComponent*>(this->GetReference() + 0x48c);
	}

	auto GetPlayerReplicationInfo() -> ATgRepInfo_Player*
	{
		return read<ATgRepInfo_Player*>(this->GetReference() + 0x440);
	}

	auto GetHealth()
	{
		return read<float>(this->GetReference() + 0x3c4);
	}

	auto GetHealthMax()
	{
		return read<float>(this->GetReference() + 0x3c8);
	}

	auto GetWeapon() -> ATgDevice
	{
		return read<ATgDevice>(this->GetReference() + 0x4E4);
	}

	auto IsOfSameTeam(APawn* Other)
	{
		if (Other == nullptr) return false;

		auto LocalRepInfo = this->GetPlayerReplicationInfo(),
			OtherRepInfo = Other->GetPlayerReplicationInfo();
		if (LocalRepInfo == nullptr || OtherRepInfo == nullptr) return false;

		auto LocalTaskForce = LocalRepInfo->GetTaskForce(),
			OtherTaskForce = OtherRepInfo->GetTaskForce();
		if (LocalTaskForce == nullptr || OtherTaskForce == nullptr) return false;

		return static_cast<bool>(LocalTaskForce->GetnTaskForce() == OtherTaskForce->GetnTaskForce());
	}
};

class AWorldInfo : public AActor
{
public:
	static auto GetStaticClass() -> UObject*
	{
		static UObject* WorldInfoClass = nullptr;
		if (WorldInfoClass == nullptr)
			WorldInfoClass = UObject::StaticFindObject(nullptr, nullptr, L"Engine.WorldInfo", false);

		return WorldInfoClass;
	}

	static auto GetWorldInfo() -> AWorldInfo*
	{
		auto WorldInfoStaticClass = UEngine::GetStaticClass();
		if (!WorldInfoStaticClass) return nullptr;

		static UObject* GetWorldInfoObject = nullptr;
		if (GetWorldInfoObject == nullptr)
		{
			GetWorldInfoObject = UObject::StaticFindObject(nullptr, nullptr, L"Engine.WorldInfo.GetWorldInfo", false);

			if (GetWorldInfoObject == nullptr) return nullptr;
		}

		struct
		{
			AWorldInfo* ReturnValue;
		} params;

		UObject::ProcessEvent(WorldInfoStaticClass, GetWorldInfoObject, reinterpret_cast<uintptr_t>(&params), 0);
		return params.ReturnValue;
	}

	auto GetPawnList()
	{
		return read<APawn*>(this->GetReference() + 0x5b4);
	}

	auto GetTimeSeconds() -> float
	{
		return read<float>(this->GetReference() + 0x4EC);
	}

	auto GetDeltaSeconds() -> float
	{
		return read<float>(this->GetReference() + 0x4F8); // AWorldInfo->DeltaSeconds
	}

	auto GetRealDeltaSeconds() -> float
	{
		return read<float>(this->GetReference() + 0x504); // AWorldInfo->m_fRealDeltaSeconds
	}
};

class ACamera : public AActor
{
public:
	struct FTPOV
	{
		struct FVector                                     Location;                                      // 0x0000 (0x000C) [0x0000000000000001] (CPF_Edit)    
		struct FRotator                                    Rotation;                                      // 0x000C (0x000C) [0x0000000000000001] (CPF_Edit)    
		float                                              FOV;                                           // 0x0018 (0x0004) [0x0000000000000001] (CPF_Edit)    
	};

	struct FTCameraCache
	{
		float                                              TimeStamp;                                     // 0x0000 (0x0004) [0x0000000000000000]               
		struct FTPOV                                       POV;                                           // 0x0004 (0x001C) [0x0000000000000000]               
	};

	auto GetCameraCachePOV()
	{
		return read<FTCameraCache>(this->GetReference() + 0x468).POV;
	}

	auto GetFOVAngle()
	{
		static UObject* GetFOVAngleObject = nullptr;
		if (GetFOVAngleObject == nullptr)
		{
			GetFOVAngleObject = UObject::StaticFindObject(nullptr, nullptr, L"Engine.Camera.GetFOVAngle", false);

			if (GetFOVAngleObject == nullptr) return 0.0f;
		}

		struct
		{
			float ReturnValue;
		} ViewOut;

		UObject::ProcessEvent(this, GetFOVAngleObject, reinterpret_cast<uintptr_t>(&ViewOut), 0);

		return ViewOut.ReturnValue;
	}
};

class APlayerController : public AActor
{
public:
	auto GetPawn()
	{
		return read<APawn*>(this->GetReference() + 0x498);
	}

	auto GetPlayerCamera()
	{
		return read<ACamera*>(this->GetReference() + 0x478);
	}
};

// Game specific...
class ATgPawn : public APawn
{
public:

};

// ScriptStruct TgGame.TgDevice.AccuracySettings
// 0x001C
struct FAccuracySettings
{
	unsigned long                                      bUsesAdvancedAccuracy : 1;                                // 0x0000(0x0004)
	float                                              fMaxAccuracy;                                             // 0x0004(0x0004)
	float                                              fMinAccuracy;                                             // 0x0008(0x0004)
	float                                              fAccuracyLossPerShot;                                     // 0x000C(0x0004)
	float                                              fAccuracyGainPerSec;                                      // 0x0010(0x0004)
	float                                              fAccuracyGainDelay;                                       // 0x0014(0x0004)
	int                                                nNumFreeShots;                                            // 0x0018(0x0004)
};

// ScriptStruct TgGame.TgDevice.RecoilSettings
// 0x0010
struct FRecoilSettings
{
	unsigned long                                      bUsesRecoil : 1;                                          // 0x0000(0x0004)
	float                                              fRecoilReductionPerSec;                                   // 0x0004(0x0004)
	float                                              fRecoilCenterDelay;                                       // 0x0008(0x0004)
	float                                              fRecoilSmoothRate;                                        // 0x000C(0x0004)
};

#define CONST_AspectRatio16x9                                    1.77778
#define CONST_InvAspectRatio4x3                                  0.75
#define CONST_ZeroRotator                                        Rot(0,0,0)
#define CONST_ZeroVector                                         Vect(0,0,0)
#define CONST_UpVector                                           Vect(0,0,1)
#define CONST_RightVector                                        Vect(0,1,0)
#define CONST_ForwardVector                                      Vect(1,0,0)
#define CONST_RadToDeg                                           57.295779513082321600
#define CONST_InvAspectRatio16x9                                 0.56249
#define CONST_InvAspectRatio5x4                                  0.8
#define CONST_AspectRatio5x4                                     1.25
#define CONST_AspectRatio4x3                                     1.33333
#define CONST_INDEX_NONE                                         -1
#define CONST_UnrRotToDeg                                        0.00549316540360483
#define CONST_DegToUnrRot                                        182.0444
#define CONST_RadToUnrRot                                        10430.3783504704527
#define CONST_UnrRotToRad                                        0.00009587379924285
#define CONST_DegToRad                                           0.017453292519943296
#define CONST_Pi                                                 3.1415926535897932
#define CONST_MinInt                                             0x80000000
#define CONST_MaxInt                                             0x7fffffff

namespace Engine
{
	void Normalize(FVector& v)
	{
		float size = v.Length();

		if (!size)
		{
			v.X = v.Y = v.Z = 1;
		}
		else
		{
			v.X /= size;
			v.Y /= size;
			v.Z /= size;
		}
	}

	FVector RotationToVector(FRotator R)
	{
		FVector Vec;
		float fYaw = R.Yaw * CONST_UnrRotToRad;
		float fPitch = R.Pitch * CONST_UnrRotToRad;
		float CosPitch = cos(fPitch);
		Vec.X = cos(fYaw) * CosPitch;
		Vec.Y = sin(fYaw) * CosPitch;
		Vec.Z = sin(fPitch);
		return Vec;
	}

	FRotator VectorToRotation(FVector vVector)
	{
		FRotator rRotation;

		rRotation.Yaw = atan2(vVector.Y, vVector.X) * CONST_RadToUnrRot;
		rRotation.Pitch = atan2(vVector.Z, sqrt((vVector.X * vVector.X) + (vVector.Y * vVector.Y))) * CONST_RadToUnrRot;
		rRotation.Roll = 0;

		return rRotation;
	}

	auto AimAtVector(FVector TargetVec, FVector PlayerLocation)
	{
		return VectorToRotation(TargetVec - PlayerLocation);
	}

	void GetAxes(FRotator R, FVector& X, FVector& Y, FVector& Z)
	{
		X = RotationToVector(R);
		Normalize(X);
		R.Yaw += 16384;
		FRotator R2 = R;
		R2.Pitch = 0.f;
		Y = RotationToVector(R2);
		Normalize(Y);
		Y.Z = 0.f;
		R.Yaw -= 16384;
		R.Pitch += 16384;
		Z = RotationToVector(R);
		Normalize(Z);
	}

	FVector VectorSubtract(FVector s1, FVector s2)
	{
		FVector temp;
		temp.X = s1.X - s2.X;
		temp.Y = s1.Y - s2.Y;
		temp.Z = s1.Z - s2.Z;

		return temp;
	}

	float VectorDotProduct(const FVector& A, const FVector& B)
	{
		float tempx = A.X * B.X;
		float tempy = A.Y * B.Y;
		float tempz = A.Z * B.Z;

		return (tempx + tempy + tempz);
	}

	FVector ProjectWorldToScreen(FVector Location)
	{
		auto Return = FVector();
		FVector AxisX{}, AxisY{}, AxisZ{}, Delta{}, Transformed{};

		auto CameraRotation = ACameraCache::GetRotation();
		auto CameraLocation = ACameraCache::GetLocation();

		GetAxes(CameraRotation, AxisX, AxisY, AxisZ);

		Delta.X = Location.X - CameraLocation.X;
		Delta.Y = Location.Y - CameraLocation.Y;
		Delta.Z = Location.Z - CameraLocation.Z;

		Transformed.X = VectorDotProduct(Delta, AxisY);
		Transformed.Y = VectorDotProduct(Delta, AxisZ);
		Transformed.Z = VectorDotProduct(Delta, AxisX);

		if (Transformed.Z < 1.00f)
			Transformed.Z = 1.00f;

		auto FOVAngle = ACameraCache::GetFOVAngle();

		Return.X = (float)(width / 2.0f) + Transformed.X * (float)((width / 2.0f) / tanf(FOVAngle * (float)CONST_Pi / 360.0f)) / Transformed.Z;
		Return.Y = (float)(height / 2.0f) + -Transformed.Y * (float)((width / 2.0f) / tanf(FOVAngle * (float)CONST_Pi / 360.0f)) / Transformed.Z;
		Return.Z = 0;

		return Return;
	}

	void Boxes(ImGuiWindow& window, APlayerController* PlayerController, ImU32 Color, FBox Returned)
	{
		FVector Min, Max, vVec1, vVec2, vVec3, vVec4, vVec5, vVec6, vVec7, vVec8;

		if (Returned.IsValid)
		{
			Max = Returned.Max;
			Min = Returned.Min;

			vVec3 = Min;
			vVec3.X = Max.X;
			vVec4 = Min;
			vVec4.Y = Max.Y;
			vVec5 = Min;
			vVec5.Z = Max.Z;
			vVec6 = Max;
			vVec6.X = Min.X;
			vVec7 = Max;
			vVec7.Y = Min.Y;
			vVec8 = Max;
			vVec8.Z = Min.Z;
			vVec1 = ProjectWorldToScreen(Min);
			vVec2 = ProjectWorldToScreen(Max);
			vVec3 = ProjectWorldToScreen(vVec3);
			vVec4 = ProjectWorldToScreen(vVec4);
			vVec5 = ProjectWorldToScreen(vVec5);
			vVec6 = ProjectWorldToScreen(vVec6);
			vVec7 = ProjectWorldToScreen(vVec7);
			vVec8 = ProjectWorldToScreen(vVec8);

			window.DrawList->AddLine(ImVec2(vVec1.X, vVec1.Y), ImVec2(vVec5.X, vVec5.Y), Color);
			window.DrawList->AddLine(ImVec2(vVec2.X, vVec2.Y), ImVec2(vVec8.X, vVec8.Y), Color);
			window.DrawList->AddLine(ImVec2(vVec3.X, vVec3.Y), ImVec2(vVec7.X, vVec7.Y), Color);
			window.DrawList->AddLine(ImVec2(vVec4.X, vVec4.Y), ImVec2(vVec6.X, vVec6.Y), Color);
			window.DrawList->AddLine(ImVec2(vVec1.X, vVec1.Y), ImVec2(vVec3.X, vVec3.Y), Color);
			window.DrawList->AddLine(ImVec2(vVec1.X, vVec1.Y), ImVec2(vVec4.X, vVec4.Y), Color);
			window.DrawList->AddLine(ImVec2(vVec8.X, vVec8.Y), ImVec2(vVec3.X, vVec3.Y), Color);
			window.DrawList->AddLine(ImVec2(vVec8.X, vVec8.Y), ImVec2(vVec4.X, vVec4.Y), Color);
			window.DrawList->AddLine(ImVec2(vVec2.X, vVec2.Y), ImVec2(vVec6.X, vVec6.Y), Color);
			window.DrawList->AddLine(ImVec2(vVec2.X, vVec2.Y), ImVec2(vVec7.X, vVec7.Y), Color);
			window.DrawList->AddLine(ImVec2(vVec5.X, vVec5.Y), ImVec2(vVec6.X, vVec6.Y), Color);
			window.DrawList->AddLine(ImVec2(vVec5.X, vVec5.Y), ImVec2(vVec7.X, vVec7.Y), Color);
		}
	}
}