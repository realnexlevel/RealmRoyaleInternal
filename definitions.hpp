#pragma once

#define M_PI 3.14159265358979323846264338327950288419716939937510f
#define RVA(Address, Size) ((uintptr_t)((UINT_PTR)(Address) + *(PINT)((UINT_PTR)(Address) + ((Size) - sizeof(INT))) + (Size)))

inline const char* SelectedHitbox = "HEAD";
inline const char* AllHitboxes[] = { "HEAD", "NECK", "CHEST" };

inline const char* SelectedAimbot = "SILENT";
inline const char* AllAimbots[] = { "MEMORY", "SILENT" };

inline int SelectedBoxStyle = 0;
inline const char* AllBoxStyles[] = { "2D", "2D-FILLED", "CORNERED", "CORNERED-FILLED", "3D" };

inline auto bIsResizing = false;
inline IDXGISwapChain* g_pSwapChain = nullptr;

inline ID3D11Device* device = nullptr;
inline ID3D11DeviceContext* immediateContext = nullptr;
inline ID3D11RenderTargetView* renderTargetView = nullptr;

inline HWND hWnd = nullptr;
inline WNDPROC WndProcOriginal = nullptr;

inline auto width = 0.0f, height = 0.0f;

template <typename DataType>
inline DataType read(uintptr_t address)
{
	if (IsBadReadPtr(reinterpret_cast<const void*>(address), sizeof(DataType)))
		return DataType{};

	return *reinterpret_cast<DataType*>(address);
}

template <typename DataType>
inline bool write(uintptr_t address, DataType ValueToWrite)
{
	if (IsBadWritePtr(reinterpret_cast<LPVOID>(address), sizeof(DataType)))
		return false;

	*reinterpret_cast<DataType*>(address) = ValueToWrite;
	return true;
}

typedef struct _D3DMATRIX
{
	union
	{
		struct
		{
			float        _11, _12, _13, _14;
			float        _21, _22, _23, _24;
			float        _31, _32, _33, _34;
			float        _41, _42, _43, _44;

		};

		float m[4][4];
	};
} D3DMATRIX;

// ScriptStruct Core.Object.Rotator
// 0x000C
struct FRotator
{
	int32_t                                            Pitch;                                         // 0x0000 (0x0004) [0x0000000000000001] (CPF_Edit)    
	int32_t                                            Yaw;                                           // 0x0004 (0x0004) [0x0000000000000001] (CPF_Edit)    
	int32_t                                            Roll;                                          // 0x0008 (0x0004) [0x0000000000000001] (CPF_Edit)  

	FRotator() : Pitch(0), Yaw(0), Roll(0) {}

	FRotator(int32_t x, int32_t y, int32_t z) : Pitch(x), Yaw(y), Roll(z) {}

	bool operator==(const FRotator& V) const { return Pitch == V.Pitch && Yaw == V.Yaw && Roll == V.Roll; }
};

// ScriptStruct CoreUObject.Vector
// Size: 0x0c (Inherited: 0x00)
struct FVector
{
	float                                              X;                                             // 0x0000 (0x0004) [0x0000000000000001] (CPF_Edit)    
	float                                              Y;                                             // 0x0004 (0x0004) [0x0000000000000001] (CPF_Edit)    
	float                                              Z;                                             // 0x0008 (0x0004) [0x0000000000000001] (CPF_Edit)    

	FVector() : X(0.f), Y(0.f), Z(0.f) {}

	FVector(float x, float y, float z) : X(x), Y(y), Z(z) {}

	FVector operator+(const FVector& other) const { return FVector(X + other.X, Y + other.Y, Z + other.Z); }

	FVector operator-(const FVector& other) const { return FVector(X - other.X, Y - other.Y, Z - other.Z); }

	FVector operator*(const FVector& V) const { return FVector(X * V.X, Y * V.Y, Z * V.Z); }

	FVector operator/(const FVector& V) const { return FVector(X / V.X, Y / V.Y, Z / V.Z); }

	bool operator==(const FVector& V) const { return X == V.X && Y == V.Y && Z == V.Z; }

	bool operator!=(const FVector& V) const { return X != V.X || Y != V.Y || Z != V.Z; }

	FVector operator-() const { return FVector(-X, -Y, -Z); }

	FVector operator+(float Bias) const { return FVector(X + Bias, Y + Bias, Z + Bias); }

	FVector operator-(float Bias) const { return FVector(X - Bias, Y - Bias, Z - Bias); }

	FVector operator*(float Scale) const { return FVector(X * Scale, Y * Scale, Z * Scale); } const

	FVector operator/(float Scale) const { const float RScale = 1.f / Scale; return FVector(X * RScale, Y * RScale, Z * RScale); }

	FVector operator-=(const FVector& V) { X -= V.X; Y -= V.Y; Z -= V.Z; return *this; }

	FVector operator*=(const FVector& V) { X *= V.X; Y *= V.Y; Z *= V.Z; return *this; }

	FVector operator/=(const FVector& V) { X /= V.X; Y /= V.Y; Z /= V.Z; return *this; }

	FVector operator*=(float Scale) { X *= Scale; Y *= Scale; Z *= Scale; return *this; }

	FVector operator/=(float V) { const float RV = 1.f / V; X *= RV; Y *= RV; Z *= RV; return *this; }

	inline float Length()
	{
		return sqrtf(X * X + Y * Y + Z * Z);
	}

	inline float Dot(FVector v)
	{
		return X * v.X + Y * v.Y + Z * v.Z;
	}

	inline float Distance(FVector v)
	{
		return sqrtf(powf(v.X - X, 2.0f) + powf(v.Y - Y, 2.0f) + powf(v.Z - Z, 2.0f));
	}

	static FORCEINLINE float InvSqrt(float F)
	{
		return 1.0f / sqrtf(F);
	}

	FORCEINLINE FVector GetSafeNormal(float Tolerance) const
	{
		const float SquareSum = X * X + Y * Y + Z * Z;

		if (SquareSum == 1.f)
		{
			return *this;
		}
		else if (SquareSum < Tolerance)
		{
			return FVector();
		}

		const float Scale = InvSqrt(SquareSum);

		return FVector(X * Scale, Y * Scale, Z * Scale);
	}
};

// ScriptStruct Core.Object.Box
// 0x0019
struct FBox
{
	struct FVector                                     Min;                                           // 0x0000 (0x000C) [0x0000000000000001] (CPF_Edit)    
	struct FVector                                     Max;                                           // 0x000C (0x000C) [0x0000000000000001] (CPF_Edit)    
	uint8_t                                            IsValid;                                       // 0x0018 (0x0001) [0x0000000000000000]               
};

// ScriptStruct CoreUObject.Plane
// Size: 0x10 (Inherited: 0x0c)
struct FPlane : FVector
{
	float W; // 0x0c(0x04)
};

// ScriptStruct CoreUObject.Matrix
// 0x0040
typedef struct FMatrix
{
	FPlane                                      XPlane;                                                    // 0x0000(0x0010) (Edit, BlueprintVisible, ZeroConstructor, SaveGame, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	FPlane                                      YPlane;                                                    // 0x0010(0x0010) (Edit, BlueprintVisible, ZeroConstructor, SaveGame, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	FPlane                                      ZPlane;                                                    // 0x0020(0x0010) (Edit, BlueprintVisible, ZeroConstructor, SaveGame, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	FPlane                                      WPlane;                                                    // 0x0030(0x0010) (Edit, BlueprintVisible, ZeroConstructor, SaveGame, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
};

namespace Player
{
	enum Bones
	{
		Root_bn,
		C_Pelvis_BN,
		C_Hip_BN,
		L_Thigh_BN,
		L_Calf_BN,
		L_Foot_bn,
		L_Toe_BN,
		L_SkydiveFoot01_BN,
		L_Calf_Twist01_BN,
		L_Calf_Twist02_BN,
		L_Calf_Twist03_BN,
		L_Thigh_Twist01_BN,
		L_Thigh_Twist02_BN,
		L_Thigh_Twist03_BN,
		R_Thigh_BN,
		R_Calf_BN,
		R_Foot_bn,
		R_Toe_BN,
		R_SkydiveFoot01_BN,
		R_Calf_Twist01_BN,
		R_Calf_Twist02_BN,
		R_Calf_Twist03_BN,
		R_Thigh_Twist01_BN,
		R_Thigh_Twist02_BN,
		R_Thigh_Twist03_BN,
		R_Skirt_Rear_BN,
		R_Skirt_Front_BN,
		L_Skirt_Rear_BN,
		L_Skirt_Front_BN,
		C_TabardsFront01_BN,
		C_TabardsFront02_BN,
		C_TabardsFront03_BN,
		C_TabardsBack01_BN,
		C_TabardsBack02_BN,
		C_TabardsBack03_BN,
		L_TabardsSide01_BN,
		L_TabardsSide02_BN,
		L_TabardsSide03_BN,
		R_TabardsSide01_BN,
		R_TabardsSide02_BN,
		R_TabardsSide03_BN,
		C_Tail01_BN,
		C_Tail02_BN,
		C_Tail03_BN,
		C_Tail04_BN,
		C_Tail05_BN,
		C_Spine01_BN,
		C_Spine02_BN,
		C_Spine03_BN,
		C_Neck01_BN,
		C_Head_BN,
		C_PonyTail01_BN,
		C_PonyTail02_BN,
		C_PonyTail03_BN,
		R_PigTail01_BN,
		R_PigTail02_BN,
		R_PigTail03_BN,
		L_PigTail01_BN,
		L_PigTail02_BN,
		L_PigTail03_BN,
		L_Clavicle_BN,
		L_UpperArm_BN,
		L_Forearm_BN,
		L_Hand_BN,
		L_Thumb_01_BN,
		L_Thumb_02_BN,
		L_Thumb_03_BN,
		L_Index_01_BN,
		L_Index_02_BN,
		L_Index_03_BN,
		L_Middle_01_BN,
		L_Middle_02_BN,
		L_Middle_03_BN,
		L_Ring_01_BN,
		L_Ring_02_BN,
		L_Ring_03_BN,
		L_Pinky_01_BN,
		L_Pinky_02_BN,
		L_Pinky_03_BN,
		L_Prop,
		L_RightAlign,
		Longbow_Root_BN,
		Longbow_COM_BN,
		Longbow_String_BN,
		Longbow_L_LimbBase_BN,
		Longbow_L_LimbTip_BN,
		Longbow_L_StringEnd_BN,
		Longbow_R_LimbBase_BN,
		Longbow_R_LimbTip_BN,
		Longbow_R_StringEnd_BN,
		Longbow_Arrow_BN,
		L_SecondHandIKAttach,
		L_IndexF_01_BN,
		L_IndexF_02_BN,
		L_IndexF_03_BN,
		L_MiddleF_01_BN,
		L_MiddleF_02_BN,
		L_MiddleF_03_BN,
		L_RingF_01_BN,
		L_RingF_02_BN,
		L_RingF_03_BN,
		L_PinkyF_01_BN,
		L_PinkyF_02_BN,
		L_PinkyF_03_BN,
		L_ThumbF_01_BN,
		L_ThumbF_02_BN,
		L_ThumbF_03_BN,
		L_SkydiveHand01_BN,
		L_Forearm_Twist01_BN,
		L_Forearm_Twist02_BN,
		L_Forearm_Twist03_BN,
		L_Forearm_Twist04_BN,
		PaladinHandShield_BN,
		L_UpperArm_Twist01_BN,
		L_UpperArm_Twist02_BN,
		L_UpperArm_Twist03_BN,
		L_UpperArm_Twist04_BN,
		L_UpperArmF_Twist01_BN,
		L_UpperArmF_Twist02_BN,
		L_UpperArmF_Twist03_BN,
		BandolierFront_BN,
		BandolierBack_BN,
		L_Pauldron_BN,
		L_PauldronF01_BN,
		R_Clavicle_BN,
		R_UpperArm_BN,
		R_Forearm_BN,
		R_Hand_BN,
		R_Thumb_01_BN,
		R_Thumb_02_BN,
		R_Thumb_03_BN,
		R_Index_01_BN,
		R_Index_02_BN,
		R_Index_03_BN,
		R_Middle_01_BN,
		R_Middle_02_BN,
		R_Middle_03_BN,
		R_Ring_01_BN,
		R_Ring_02_BN,
		R_Ring_03_BN,
		R_Pinky_01_BN,
		R_Pinky_02_BN,
		R_Pinky_03_BN,
		R_Prop,
		R_LeftAlign,
		SecondHandIK,
		Longbow_ArrowHand_BN,
		R_ThumbF_01_BN,
		R_ThumbF_02_BN,
		R_ThumbF_03_BN,
		R_IndexF_01_BN,
		R_IndexF_02_BN,
		R_IndexF_03_BN,
		R_MiddleF_01_BN,
		R_MiddleF_02_BN,
		R_MiddleF_03_BN,
		R_RingF_01_BN,
		R_RingF_02_BN,
		R_RingF_03_BN,
		R_PinkyF_01_BN,
		R_PinkyF_02_BN,
		R_PinkyF_03_BN,
		R_SkydiveHand01_BN,
		R_Forearm_Twist01_BN,
		R_Forearm_Twist02_BN,
		R_Forearm_Twist03_BN,
		R_Forearm_Twist04_BN,
		R_UpperArm_Twist01_BN,
		R_UpperArm_Twist02_BN,
		R_UpperArm_Twist03_BN,
		R_UpperArm_Twist04_BN,
		R_UpperArmF_Twist03_BN,
		R_UpperArmF_Twist02_BN,
		R_UpperArmF_Twist01_BN,
		R_Pauldron_BN,
		R_PauldronF01_BN,
		C_Cape01_BN,
		C_Cape02_BN,
		C_Cape03_BN,
		C_Cape04_BN,
		PaladinShield_BN,
		L_WingShoulder01_BN,
		L_WingElbow01_BN,
		L_WingElbowFlap01_BN,
		L_WingForearm01_BN,
		L_WingWrist01_BN,
		L_WingFingerTop01_BN,
		L_WingFingerTop02_BN,
		L_WingFingerMid01_BN,
		L_WingFingerMid02_BN,
		L_WingFingerBot01_BN,
		L_WingFingerBot02_BN,
		R_WingShoulder01_BN,
		R_WingElbow01_BN,
		R_WingElbowFlap01_BN,
		R_WingForearm01_BN,
		R_WingWrist01_BN,
		R_WingFingerTop01_BN,
		R_WingFingerTop02_BN,
		R_WingFingerMid01_BN,
		R_WingFingerMid02_BN,
		R_WingFingerBot01_BN,
		R_WingFingerBot02_BN,
		JetPack_Root_BN,
		COM_bn,
		L_RocketBase_BN,
		L_Rocket_BN,
		L_RocketTip_BN,
		R_RocketBase_BN,
		R_Rocket_BN,
		R_RocketTip_BN,
		C_SkydiveBody01_BN,
		COG,
		L_Prop_SC,
		R_Prop_SC,
		C_Prop,
		C_NoneAlign,
		C_Prop_SC,
		R_FootIK,
		L_FootIK,
		IK_Hand_Root,
		R_HandPlacement_Rot,
		C_Spine03_ROT,
		R_Clavicle_ROT,
		R_UpperArm_ROT,
		R_Forearm_ROT,
		R_HandPlacement_Pos
	};
}

static const char* KeyNames[] =
{
	"",
	"Left Mouse",
	"Right Mouse",
	"Cancel",
	"Middle Mouse",
	"Mouse 5",
	"Mouse 4",
	"",
	"Backspace",
	"Tab",
	"",
	"",
	"Clear",
	"Enter",
	"",
	"",
	"Shift",
	"Control",
	"Alt",
	"Pause",
	"Caps",
	"",
	"",
	"",
	"",
	"",
	"",
	"Escape",
	"",
	"",
	"",
	"",
	"Space",
	"Page Up",
	"Page Down",
	"End",
	"Home",
	"Left",
	"Up",
	"Right",
	"Down",
	"",
	"",
	"",
	"Print",
	"Insert",
	"Delete",
	"",
	"0",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"A",
	"B",
	"C",
	"D",
	"E",
	"F",
	"G",
	"H",
	"I",
	"J",
	"K",
	"L",
	"M",
	"N",
	"O",
	"P",
	"Q",
	"R",
	"S",
	"T",
	"U",
	"V",
	"W",
	"X",
	"Y",
	"Z",
	"",
	"",
	"",
	"",
	"",
	"Numpad 0",
	"Numpad 1",
	"Numpad 2",
	"Numpad 3",
	"Numpad 4",
	"Numpad 5",
	"Numpad 6",
	"Numpad 7",
	"Numpad 8",
	"Numpad 9",
	"Multiply",
	"Add",
	"",
	"Subtract",
	"Decimal",
	"Divide",
	"F1",
	"F2",
	"F3",
	"F4",
	"F5",
	"F6",
	"F7",
	"F8",
	"F9",
	"F10",
	"F11",
	"F12"
};

namespace Settings
{
	bool bAimbot = false, bTargetVischeck = false,
		bPlayerBox = false, bPlayerLine = false, bPlayerDistance = false,
		bPlayerSkeleton = false, bPlayerName = false, bPlayerHead = false,
		bDrawFOV = true, bDrawCrosshair = true;

	auto bNoSpread = false,
		bNoRecoil = false,
		bInstantMount = false;

	auto bLocalSpeedhack = false;
	auto SpeedHackValue = 0.0f;

	auto TargetingKey = 0;
	auto bOptionsEnabled = true;

	float CameraFOV = 0.0f, TargetingFOV = 120.0f,
		PlayerVisibleColor[3] = { 1.0f, 1.0f, 1.0f },
		PlayerInvisibleColor[3] = { 0.0f, 1.0f, 0.9019607843137255f };

	float TargetingSpeed = 10.0f;

	uintptr_t GameBaseAddress = 0;
	DWORD GameSizeOfImage = 0;
}