#include "RollBallWidgetBuilderCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"

#include "RollBall/Game/RollBallMenuWidget.h"
#include "RollBall/UI/RollBallHudWidget.h"
#include "RollBall/UI/RollBallSkillGraphWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogRollBallWidgetBuilder, Log, All);

namespace
{
	// UMG 는 화면 짧은 변이 1080 일 때 1배로 그린다. 예전 NativePaint 는 720 기준이라 그 숫자에 1.5 를 곱했다.

	const TCHAR* UiFolder = TEXT("/Game/UI");

	const FLinearColor TextColor(0.92f, 0.94f, 0.98f);
	const FLinearColor DimTextColor(0.60f, 0.66f, 0.76f);
	const FLinearColor GoldColor(0.98f, 0.80f, 0.30f);
	const FLinearColor PanelColor(0.03f, 0.035f, 0.05f, 0.82f);
	const FLinearColor NormalPhaseColor(0.30f, 0.60f, 0.90f);
	const FLinearColor HealthColor(0.95f, 0.30f, 0.35f);
	const FLinearColor FailedColor(0.95f, 0.35f, 0.35f);

	FSlateFontInfo MakeFont(float Size, bool bBold, int32 Outline = 1)
	{
		static const UObject* Roboto = LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));

		FSlateFontInfo Font(Roboto, Size, bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular")));
		Font.OutlineSettings.OutlineSize = Outline;
		Font.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.75f);
		return Font;
	}

	FSlateBrush SolidBrush(const FLinearColor& Color)
	{
		FSlateBrush Brush;
		Brush.TintColor = FSlateColor(Color);
		return Brush;
	}

	FSlateBrush RoundedBrush(const FLinearColor& Fill, const FLinearColor& Outline, float OutlineWidth, float Radius)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Fill);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(Radius, FSlateColor(Outline), OutlineWidth);
		return Brush;
	}

	FButtonStyle MenuButtonStyle()
	{
		const FLinearColor IdleFill(0.07f, 0.08f, 0.11f);
		const FLinearColor HoverFill(0.12f, 0.14f, 0.18f);
		const FLinearColor PressFill(0.05f, 0.06f, 0.08f);
		const FLinearColor IdleEdge(0.26f, 0.30f, 0.38f);
		const FLinearColor HoverEdge(0.95f, 0.96f, 1.0f);

		FButtonStyle Style;
		Style.SetNormal(RoundedBrush(IdleFill, IdleEdge, 4.0f, 6.0f));
		Style.SetHovered(RoundedBrush(HoverFill, HoverEdge, 4.0f, 6.0f));
		Style.SetPressed(RoundedBrush(PressFill, HoverEdge, 4.0f, 6.0f));
		Style.SetDisabled(RoundedBrush(IdleFill * 0.7f, IdleEdge * 0.7f, 4.0f, 6.0f));
		Style.SetNormalPadding(FMargin(0.0f));
		Style.SetPressedPadding(FMargin(0.0f));
		return Style;
	}

	/** 위젯 트리에 위젯을 만들어 넣을 때마다 디자이너가 하는 뒷정리(GUID 등록)를 같이 한다 */
	class FTreeBuilder
	{
	public:
		explicit FTreeBuilder(UWidgetBlueprint* InBlueprint)
			: Blueprint(InBlueprint)
			, Tree(InBlueprint->WidgetTree)
		{
		}

		template <typename T>
		T* Make(const FString& Name, bool bVariable = false)
		{
			T* Widget = Tree->ConstructWidget<T>(T::StaticClass(), FName(*Name));
			Widget->bIsVariable = bVariable;
			Blueprint->OnVariableAdded(Widget->GetFName());
			return Widget;
		}

		UTextBlock* Text(const FString& Name, const FString& Value, float Size, bool bBold,
			const FLinearColor& Color, ETextJustify::Type Justify, bool bVariable = false, int32 Outline = 1)
		{
			UTextBlock* Block = Make<UTextBlock>(Name, bVariable);
			Block->SetText(FText::FromString(Value));
			Block->SetFont(MakeFont(Size, bBold, Outline));
			Block->SetColorAndOpacity(FSlateColor(Color));
			Block->SetJustification(Justify);
			return Block;
		}

		UButton* Button(const FString& Name, const FString& LabelName, const FString& Label,
			float Width, float Height, float FontSize, bool bLabelVariable, USizeBox*& OutSize)
		{
			OutSize = Make<USizeBox>(Name + TEXT("Size"));
			OutSize->SetWidthOverride(Width);
			OutSize->SetHeightOverride(Height);

			UButton* NewButton = Make<UButton>(Name, true);
			NewButton->SetStyle(MenuButtonStyle());

			UTextBlock* LabelBlock = Text(LabelName, Label, FontSize, true,
				FLinearColor(0.90f, 0.93f, 0.98f), ETextJustify::Center, bLabelVariable);

			if (UButtonSlot* LabelSlot = Cast<UButtonSlot>(NewButton->AddChild(LabelBlock)))
			{
				LabelSlot->SetPadding(FMargin(0.0f));
				LabelSlot->SetHorizontalAlignment(HAlign_Center);
				LabelSlot->SetVerticalAlignment(VAlign_Center);
			}

			OutSize->AddChild(NewButton);
			return NewButton;
		}

		void SetRoot(UWidget* Root)
		{
			Tree->RootWidget = Root;
		}

	private:
		UWidgetBlueprint* Blueprint;
		UWidgetTree* Tree;
	};

	UCanvasPanelSlot* PlaceFill(UCanvasPanel* Canvas, UWidget* Widget)
	{
		UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Widget);
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		CanvasSlot->SetOffsets(FMargin(0.0f));
		return CanvasSlot;
	}

	/** 위쪽에 가로로 꽉 차게 붙인다 */
	UCanvasPanelSlot* PlaceTopStrip(UCanvasPanel* Canvas, UWidget* Widget, float Top, float Height)
	{
		UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Widget);
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 0.0f));
		CanvasSlot->SetOffsets(FMargin(0.0f, Top, 0.0f, Height));
		return CanvasSlot;
	}

	UVerticalBoxSlot* AddVertical(UVerticalBox* Box, UWidget* Widget, const FMargin& Padding,
		EHorizontalAlignment HAlign = HAlign_Fill)
	{
		UVerticalBoxSlot* BoxSlot = Box->AddChildToVerticalBox(Widget);
		BoxSlot->SetPadding(Padding);
		BoxSlot->SetHorizontalAlignment(HAlign);
		return BoxSlot;
	}

	UHorizontalBoxSlot* AddHorizontal(UHorizontalBox* Box, UWidget* Widget, const FMargin& Padding,
		bool bFill = false)
	{
		UHorizontalBoxSlot* BoxSlot = Box->AddChildToHorizontalBox(Widget);
		BoxSlot->SetPadding(Padding);
		BoxSlot->SetVerticalAlignment(VAlign_Center);
		BoxSlot->SetSize(FSlateChildSize(bFill ? ESlateSizeRule::Fill : ESlateSizeRule::Automatic));
		return BoxSlot;
	}

	UOverlaySlot* AddOverlay(UOverlay* Overlay, UWidget* Widget, EHorizontalAlignment HAlign)
	{
		UOverlaySlot* OverlaySlot = Overlay->AddChildToOverlay(Widget);
		OverlaySlot->SetHorizontalAlignment(HAlign);
		OverlaySlot->SetVerticalAlignment(VAlign_Center);
		return OverlaySlot;
	}

	/**
	 * 에셋을 찾거나 새로 만든다. 이미 있고 bForce 가 아니면 bOutShouldBuild 가 false 다.
	 * bForce 면 기존 에셋을 그대로 두고 트리만 비운다. 에셋을 지웠다 새로 만들면 다른 곳의 참조가 끊긴다.
	 */
	UWidgetBlueprint* FindOrCreate(const FString& AssetName, UClass* ParentClass, bool bForce, bool& bOutShouldBuild)
	{
		const FString PackageName = FString(UiFolder) / AssetName;
		const FString ObjectPath = PackageName + TEXT(".") + AssetName;

		if (UWidgetBlueprint* Existing = LoadObject<UWidgetBlueprint>(nullptr, *ObjectPath, nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			bOutShouldBuild = bForce;

			if (bForce)
			{
				TArray<UWidget*> OldWidgets;
				Existing->WidgetTree->GetAllWidgets(OldWidgets);

				for (UWidget* Old : OldWidgets)
				{
					Old->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
				}

				Existing->WidgetTree->RootWidget = nullptr;
				Existing->WidgetVariableNameToGuidMap.Empty();
			}

			return Existing;
		}

		UPackage* Package = CreatePackage(*PackageName);
		UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(
			ParentClass, Package, FName(*AssetName), BPTYPE_Normal,
			UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass()));

		if (Blueprint != nullptr)
		{
			FAssetRegistryModule::AssetCreated(Blueprint);
		}

		bOutShouldBuild = true;
		return Blueprint;
	}

	bool Save(UWidgetBlueprint* Blueprint)
	{
		UPackage* Package = Blueprint->GetOutermost();
		Package->MarkPackageDirty();

		const FString Filename = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;

		const bool bSaved = UPackage::SavePackage(Package, Blueprint, *Filename, SaveArgs);
		UE_LOG(LogRollBallWidgetBuilder, Display, TEXT("%s 저장 %s"), *Filename, bSaved ? TEXT("성공") : TEXT("실패"));
		return bSaved;
	}

	bool CompileAndSave(UWidgetBlueprint* Blueprint)
	{
		FKismetEditorUtilities::CompileBlueprint(Blueprint);

		if (Blueprint->Status == BS_Error)
		{
			UE_LOG(LogRollBallWidgetBuilder, Error, TEXT("%s 컴파일 실패"), *Blueprint->GetName());
			return false;
		}

		return Save(Blueprint);
	}

	void BuildSkillTree(UWidgetBlueprint* Blueprint)
	{
		FTreeBuilder B(Blueprint);

		UCanvasPanel* Root = B.Make<UCanvasPanel>(TEXT("Root"));
		B.SetRoot(Root);

		// 그래프는 C++ NativePaint 가 이 위젯 전체에 그린다. 여기에는 그 위에 얹는 머리 줄만 둔다.
		UBorder* Header = B.Make<UBorder>(TEXT("Header"));
		Header->SetBrush(SolidBrush(FLinearColor(0.02f, 0.02f, 0.03f, 0.92f)));
		Header->SetPadding(FMargin(36.0f, 0.0f, 24.0f, 0.0f));
		Header->SetVerticalAlignment(VAlign_Center);
		PlaceTopStrip(Root, Header, 0.0f, 84.0f);

		UHorizontalBox* Row = B.Make<UHorizontalBox>(TEXT("HeaderRow"));
		Header->AddChild(Row);

		AddHorizontal(Row, B.Text(TEXT("HeaderTitle"), TEXT("스킬 트리"), 36.0f, true, TextColor, ETextJustify::Left),
			FMargin(0.0f));

		AddHorizontal(Row, B.Text(TEXT("HeaderHint"), TEXT("휠: 확대    드래그: 이동    Home: 전체 보기    Esc: 닫기"),
			20.0f, false, DimTextColor, ETextJustify::Left), FMargin(36.0f, 0.0f, 0.0f, 0.0f), true);

		AddHorizontal(Row, B.Text(TEXT("GoldText"), TEXT("보유 골드 0"), 30.0f, true, GoldColor, ETextJustify::Right, true),
			FMargin(0.0f, 0.0f, 30.0f, 0.0f));

		USizeBox* CloseSize = nullptr;
		B.Button(TEXT("CloseButton"), TEXT("CloseLabel"), TEXT("닫기"), 150.0f, 54.0f, 26.0f, false, CloseSize);
		AddHorizontal(Row, CloseSize, FMargin(0.0f));
	}

	void BuildMainMenu(UWidgetBlueprint* Blueprint)
	{
		FTreeBuilder B(Blueprint);

		UCanvasPanel* Root = B.Make<UCanvasPanel>(TEXT("Root"));
		B.SetRoot(Root);

		UBorder* Background = B.Make<UBorder>(TEXT("Background"));
		Background->SetBrush(SolidBrush(FLinearColor(0.035f, 0.04f, 0.055f, 1.0f)));
		PlaceFill(Root, Background);

		UVerticalBox* MenuBox = B.Make<UVerticalBox>(TEXT("MenuBox"));
		UCanvasPanelSlot* MenuSlot = Root->AddChildToCanvas(MenuBox);
		MenuSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		MenuSlot->SetAlignment(FVector2D(0.5, 0.5));
		MenuSlot->SetOffsets(FMargin(0.0f));
		MenuSlot->SetAutoSize(true);

		AddVertical(MenuBox, B.Text(TEXT("TitleText"), TEXT("ROLL BALL"), 96.0f, true,
			FLinearColor(0.95f, 0.96f, 1.0f), ETextJustify::Center, false, 2), FMargin(0.0f), HAlign_Center);

		AddVertical(MenuBox, B.Text(TEXT("AccountText"), TEXT("골드 0      최고 스테이지 0      환생 0회"), 27.0f, false,
			FLinearColor(0.62f, 0.68f, 0.80f), ETextJustify::Center, true), FMargin(0.0f, 0.0f, 0.0f, 69.0f), HAlign_Center);

		struct FItem
		{
			const TCHAR* Button;
			const TCHAR* Label;
			const TCHAR* Text;
			bool bLabelVariable;
		};

		const FItem Items[] = {
			{ TEXT("StartButton"),     TEXT("StartLabel"),     TEXT("게임 시작"),        false },
			{ TEXT("SkillTreeButton"), TEXT("SkillTreeLabel"), TEXT("스킬 트리"),        false },
			{ TEXT("RebirthButton"),   TEXT("RebirthLabel"),   TEXT("환생  (+0 골드)"), true },
			{ TEXT("DeleteButton"),    TEXT("DeleteLabel"),    TEXT("계정 삭제"),        true },
		};

		for (int32 i = 0; i < UE_ARRAY_COUNT(Items); ++i)
		{
			USizeBox* Size = nullptr;
			B.Button(Items[i].Button, Items[i].Label, Items[i].Text, 630.0f, 99.0f, 39.0f, Items[i].bLabelVariable, Size);

			const bool bLast = (i + 1 == UE_ARRAY_COUNT(Items));
			AddVertical(MenuBox, Size, FMargin(0.0f, 0.0f, 0.0f, bLast ? 0.0f : 24.0f), HAlign_Center);
		}
	}

	void BuildHud(UWidgetBlueprint* Blueprint)
	{
		FTreeBuilder B(Blueprint);

		UCanvasPanel* Root = B.Make<UCanvasPanel>(TEXT("Root"));
		B.SetRoot(Root);

		// 상단 바: 왼쪽 스테이지·페이즈, 가운데 시계·남은 시간 막대, 오른쪽 골드·처치
		UBorder* TopBar = B.Make<UBorder>(TEXT("TopBar"));
		TopBar->SetBrush(SolidBrush(PanelColor));
		TopBar->SetPadding(FMargin(36.0f, 0.0f, 36.0f, 0.0f));
		PlaceTopStrip(Root, TopBar, 0.0f, 126.0f);

		UOverlay* Content = B.Make<UOverlay>(TEXT("TopBarContent"));
		TopBar->AddChild(Content);

		UVerticalBox* Left = B.Make<UVerticalBox>(TEXT("LeftBox"));
		AddOverlay(Content, Left, HAlign_Left);
		AddVertical(Left, B.Text(TEXT("StageText"), TEXT("스테이지 1"), 33.0f, true, TextColor, ETextJustify::Left, true), FMargin(0.0f));
		AddVertical(Left, B.Text(TEXT("PhaseText"), TEXT("일반"), 22.0f, false, NormalPhaseColor, ETextJustify::Left, true),
			FMargin(0.0f, 3.0f, 0.0f, 0.0f));

		UVerticalBox* Centre = B.Make<UVerticalBox>(TEXT("CenterBox"));
		AddOverlay(Content, Centre, HAlign_Center);
		AddVertical(Centre, B.Text(TEXT("ClockText"), TEXT("12:00"), 63.0f, true, TextColor, ETextJustify::Center, true),
			FMargin(0.0f), HAlign_Center);

		USizeBox* BarSize = B.Make<USizeBox>(TEXT("TimeBarSize"));
		BarSize->SetWidthOverride(630.0f);
		BarSize->SetHeightOverride(10.0f);
		AddVertical(Centre, BarSize, FMargin(0.0f, 2.0f, 0.0f, 0.0f), HAlign_Center);

		UProgressBar* TimeBar = B.Make<UProgressBar>(TEXT("TimeBar"), true);
		FProgressBarStyle BarStyle;
		BarStyle.SetBackgroundImage(SolidBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.14f)));
		BarStyle.SetFillImage(SolidBrush(FLinearColor::White));
		BarStyle.SetMarqueeImage(SolidBrush(FLinearColor::White));
		TimeBar->SetWidgetStyle(BarStyle);
		TimeBar->SetFillColorAndOpacity(NormalPhaseColor);
		TimeBar->SetPercent(0.8f);
		BarSize->AddChild(TimeBar);

		UVerticalBox* Right = B.Make<UVerticalBox>(TEXT("RightBox"));
		AddOverlay(Content, Right, HAlign_Right);
		AddVertical(Right, B.Text(TEXT("GoldText"), TEXT("$ 0"), 33.0f, true, GoldColor, ETextJustify::Right, true),
			FMargin(0.0f), HAlign_Right);
		AddVertical(Right, B.Text(TEXT("KillText"), TEXT("처치 0"), 22.0f, false, DimTextColor, ETextJustify::Right, true),
			FMargin(0.0f, 3.0f, 0.0f, 0.0f), HAlign_Right);

		UBorder* PhaseLine = B.Make<UBorder>(TEXT("PhaseLine"), true);
		PhaseLine->SetBrush(SolidBrush(NormalPhaseColor));
		PlaceTopStrip(Root, PhaseLine, 126.0f, 6.0f);

		// 체력 칸: 첫 칸이 견본이다. 게임 중에는 C++ 가 이 칸을 본떠 최대 체력만큼 다시 만든다.
		UHorizontalBox* HealthBox = B.Make<UHorizontalBox>(TEXT("HealthBox"), true);
		UCanvasPanelSlot* HealthSlot = Root->AddChildToCanvas(HealthBox);
		HealthSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		HealthSlot->SetPosition(FVector2D(36.0, 156.0));
		HealthSlot->SetAutoSize(true);

		for (int32 i = 0; i < 3; ++i)
		{
			USizeBox* PipSize = B.Make<USizeBox>(FString::Printf(TEXT("HealthPip_%d"), i));
			PipSize->SetWidthOverride(39.0f);
			PipSize->SetHeightOverride(39.0f);

			UBorder* PipFill = B.Make<UBorder>(FString::Printf(TEXT("HealthPipFill_%d"), i));
			PipFill->SetBrush(RoundedBrush(HealthColor, FLinearColor(0.0f, 0.0f, 0.0f, 0.55f), 3.0f, 4.0f));
			PipSize->AddChild(PipFill);

			UHorizontalBoxSlot* PipSlot = HealthBox->AddChildToHorizontalBox(PipSize);
			PipSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));
		}

		// 결과 창: 평소에는 접혀 있다. 디자이너에서 보려면 계층 창에서 ResultOverlay 의 눈 아이콘을 켠다.
		UBorder* Overlay = B.Make<UBorder>(TEXT("ResultOverlay"), true);
		Overlay->SetBrush(SolidBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f)));
		Overlay->SetHorizontalAlignment(HAlign_Center);
		Overlay->SetVerticalAlignment(VAlign_Center);
		Overlay->SetVisibility(ESlateVisibility::Collapsed);
		PlaceFill(Root, Overlay);

		USizeBox* PanelSize = B.Make<USizeBox>(TEXT("ResultPanelSize"));
		PanelSize->SetWidthOverride(780.0f);
		PanelSize->SetHeightOverride(450.0f);
		Overlay->AddChild(PanelSize);

		UBorder* Panel = B.Make<UBorder>(TEXT("ResultPanel"), true);
		Panel->SetBrush(RoundedBrush(FLinearColor(0.04f, 0.045f, 0.06f, 0.96f), FailedColor, 6.0f, 10.0f));
		Panel->SetPadding(FMargin(30.0f, 48.0f, 30.0f, 30.0f));
		Panel->SetHorizontalAlignment(HAlign_Fill);
		Panel->SetVerticalAlignment(VAlign_Top);
		PanelSize->AddChild(Panel);

		UVerticalBox* ResultBox = B.Make<UVerticalBox>(TEXT("ResultBox"));
		Panel->AddChild(ResultBox);

		AddVertical(ResultBox, B.Text(TEXT("ResultTitle"), TEXT("사망"), 60.0f, true, FailedColor, ETextJustify::Center, true),
			FMargin(0.0f, 0.0f, 0.0f, 30.0f), HAlign_Center);
		AddVertical(ResultBox, B.Text(TEXT("ResultStageText"), TEXT("스테이지 1"), 30.0f, false, TextColor, ETextJustify::Center, true),
			FMargin(0.0f, 0.0f, 0.0f, 15.0f), HAlign_Center);
		AddVertical(ResultBox, B.Text(TEXT("ResultKillText"), TEXT("처치 0"), 30.0f, false, TextColor, ETextJustify::Center, true),
			FMargin(0.0f, 0.0f, 0.0f, 15.0f), HAlign_Center);
		AddVertical(ResultBox, B.Text(TEXT("ResultGoldText"), TEXT("얻은 골드 0"), 30.0f, false, GoldColor, ETextJustify::Center, true),
			FMargin(0.0f, 0.0f, 0.0f, 36.0f), HAlign_Center);
		AddVertical(ResultBox, B.Text(TEXT("ResultHintText"), TEXT("곧 이 스테이지 다시"), 22.0f, false, DimTextColor, ETextJustify::Center, true),
			FMargin(0.0f), HAlign_Center);
	}
}

URollBallWidgetBuilderCommandlet::URollBallWidgetBuilderCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 URollBallWidgetBuilderCommandlet::Main(const FString& Params)
{
	const bool bForce = FParse::Param(*Params, TEXT("force"));
	int32 Failures = 0;

	auto Run = [&](const TCHAR* Name, UClass* Parent, void (*Build)(UWidgetBlueprint*)) -> UWidgetBlueprint*
	{
		bool bShouldBuild = false;
		UWidgetBlueprint* Blueprint = FindOrCreate(Name, Parent, bForce, bShouldBuild);

		if (Blueprint == nullptr)
		{
			UE_LOG(LogRollBallWidgetBuilder, Error, TEXT("%s 를 만들지 못했습니다"), Name);
			++Failures;
			return nullptr;
		}

		if (!bShouldBuild)
		{
			UE_LOG(LogRollBallWidgetBuilder, Display, TEXT("%s 는 이미 있어서 그대로 둡니다 (-force 로 다시 짤 수 있음)"), Name);
			return Blueprint;
		}

		Build(Blueprint);

		if (!CompileAndSave(Blueprint))
		{
			++Failures;
		}

		return Blueprint;
	};

	UWidgetBlueprint* SkillTree = Run(TEXT("WBP_SkillTree"), URollBallSkillGraphWidget::StaticClass(), &BuildSkillTree);
	UWidgetBlueprint* MainMenu = Run(TEXT("WBP_MainMenu"), URollBallMenuWidget::StaticClass(), &BuildMainMenu);
	Run(TEXT("WBP_Hud"), URollBallHudWidget::StaticClass(), &BuildHud);

	// 메뉴의 스킬 트리 버튼이 WBP_SkillTree 를 열게 클래스 기본값에 넣는다.
	if (MainMenu != nullptr && SkillTree != nullptr && MainMenu->GeneratedClass != nullptr && SkillTree->GeneratedClass != nullptr)
	{
		if (FClassProperty* Property = FindFProperty<FClassProperty>(MainMenu->GeneratedClass, TEXT("SkillTreeWidgetClass")))
		{
			UObject* Defaults = MainMenu->GeneratedClass->GetDefaultObject();

			if (Property->GetObjectPropertyValue_InContainer(Defaults) != SkillTree->GeneratedClass)
			{
				Property->SetObjectPropertyValue_InContainer(Defaults, SkillTree->GeneratedClass);

				// 다시 컴파일하면 클래스 기본값을 새로 만들 수 있으니 저장만 한다.
				if (!Save(MainMenu))
				{
					++Failures;
				}
			}
		}
	}

	UE_LOG(LogRollBallWidgetBuilder, Display, TEXT("위젯 블루프린트 생성 끝. 실패 %d"), Failures);
	return Failures == 0 ? 0 : 1;
}
