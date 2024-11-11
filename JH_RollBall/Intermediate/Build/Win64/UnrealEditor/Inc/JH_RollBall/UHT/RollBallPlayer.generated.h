// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "RollBallPlayer.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
class AActor;
class UPrimitiveComponent;
struct FHitResult;
#ifdef JH_ROLLBALL_RollBallPlayer_generated_h
#error "RollBallPlayer.generated.h already included, missing '#pragma once' in RollBallPlayer.h"
#endif
#define JH_ROLLBALL_RollBallPlayer_generated_h

#define FID_JinHyung_Programming_GameProject_UE5_RollBall_JH_RollBall_Source_JH_RollBall_RollBallPlayer_h_12_RPC_WRAPPERS_NO_PURE_DECLS \
	DECLARE_FUNCTION(execOnHit);


#define FID_JinHyung_Programming_GameProject_UE5_RollBall_JH_RollBall_Source_JH_RollBall_RollBallPlayer_h_12_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesARollBallPlayer(); \
	friend struct Z_Construct_UClass_ARollBallPlayer_Statics; \
public: \
	DECLARE_CLASS(ARollBallPlayer, APawn, COMPILED_IN_FLAGS(0 | CLASS_Config), CASTCLASS_None, TEXT("/Script/JH_RollBall"), NO_API) \
	DECLARE_SERIALIZER(ARollBallPlayer)


#define FID_JinHyung_Programming_GameProject_UE5_RollBall_JH_RollBall_Source_JH_RollBall_RollBallPlayer_h_12_ENHANCED_CONSTRUCTORS \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	ARollBallPlayer(ARollBallPlayer&&); \
	ARollBallPlayer(const ARollBallPlayer&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, ARollBallPlayer); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(ARollBallPlayer); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(ARollBallPlayer) \
	NO_API virtual ~ARollBallPlayer();


#define FID_JinHyung_Programming_GameProject_UE5_RollBall_JH_RollBall_Source_JH_RollBall_RollBallPlayer_h_9_PROLOG
#define FID_JinHyung_Programming_GameProject_UE5_RollBall_JH_RollBall_Source_JH_RollBall_RollBallPlayer_h_12_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_JinHyung_Programming_GameProject_UE5_RollBall_JH_RollBall_Source_JH_RollBall_RollBallPlayer_h_12_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_JinHyung_Programming_GameProject_UE5_RollBall_JH_RollBall_Source_JH_RollBall_RollBallPlayer_h_12_INCLASS_NO_PURE_DECLS \
	FID_JinHyung_Programming_GameProject_UE5_RollBall_JH_RollBall_Source_JH_RollBall_RollBallPlayer_h_12_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> JH_ROLLBALL_API UClass* StaticClass<class ARollBallPlayer>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_JinHyung_Programming_GameProject_UE5_RollBall_JH_RollBall_Source_JH_RollBall_RollBallPlayer_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
