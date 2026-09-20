using UnrealBuildTool;
using System.Collections.Generic;

public class RollBallEditorTarget : TargetRules
{
	public RollBallEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.AddRange( new string[] { "RollBall" } );
	}
}
