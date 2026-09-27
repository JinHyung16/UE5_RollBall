using System.IO;
using UnrealBuildTool;

// 에디터에서만 쓰는 도구 모듈. 게임 빌드에는 들어가지 않는다.
public class RollBallEditor : ModuleRules
{
	public RollBallEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, ".."));

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetRegistry",
			"Slate",
			"SlateCore",
			"UMG",
			"UMGEditor",
			"UnrealEd",
			"RollBall",
		});
	}
}
