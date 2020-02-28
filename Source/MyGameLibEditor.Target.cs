// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class MyGameLibEditorTarget : TargetRules
{
	public MyGameLibEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;

		ExtraModuleNames.AddRange( new string[] { "MyGameLib" } );
	}
}
