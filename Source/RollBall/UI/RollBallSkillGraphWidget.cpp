#include "RollBallSkillGraphWidget.h"

#include "RollBallPaint.h"

#include "RollBall/Game/RollBallGameInstance.h"
#include "RollBall/Skill/RollBallSkillGraph.h"

#define LOCTEXT_NAMESPACE "RollBallSkill"

using namespace RollBallPaint;

URollBallSkillGraphWidget::URollBallSkillGraphWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

	LabelFont = FCoreStyle::GetDefaultFontStyle("Regular", 10);
	TooltipFont = FCoreStyle::GetDefaultFontStyle("Regular", 12);

	SetVisibility(ESlateVisibility::Visible);

	SetIsFocusable(true);
}

void URollBallSkillGraphWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (URollBallGameInstance* GameInstance = GetRollBallGameInstance())
	{
		SetGraph(GameInstance->EnsureSkillGraph());
	}

	SetKeyboardFocus();
}

void URollBallSkillGraphWidget::NativeDestruct()
{
	if (Graph != nullptr)
	{
		Graph->OnGraphChanged.RemoveDynamic(this, &URollBallSkillGraphWidget::HandleGraphChanged);
	}

	Super::NativeDestruct();
}

void URollBallSkillGraphWidget::SetGraph(URollBallSkillGraph* InGraph)
{
	if (Graph == InGraph)
	{
		return;
	}

	if (Graph != nullptr)
	{
		Graph->OnGraphChanged.RemoveDynamic(this, &URollBallSkillGraphWidget::HandleGraphChanged);
	}

	Graph = InGraph;
	SelectedNodeIndex = INDEX_NONE;
	HoveredNodeIndex = INDEX_NONE;
	PendingPath.Reset();

	if (Graph != nullptr)
	{
		Graph->OnGraphChanged.AddDynamic(this, &URollBallSkillGraphWidget::HandleGraphChanged);
		bPendingFrameAll = true;
	}
}

void URollBallSkillGraphWidget::HandleGraphChanged()
{
	RefreshPendingPath();
	Invalidate(EInvalidateWidgetReason::Paint);
}

FVector2D URollBallSkillGraphWidget::CellToLocal(const FVector2D& Cell, const FVector2D& CanvasSize) const
{
	const float Pixels = PixelsPerCell();
	const FVector2D Centre = CanvasSize * 0.5;

	return FVector2D(
		Centre.X + (Cell.X - Pan.X) * Pixels,
		Centre.Y - (Cell.Y - Pan.Y) * Pixels);
}

FVector2D URollBallSkillGraphWidget::LocalToCell(const FVector2D& Local, const FVector2D& CanvasSize) const
{

	const float Pixels = FMath::Max(0.0001f, PixelsPerCell());
	const FVector2D Centre = CanvasSize * 0.5;

	return FVector2D(
		Pan.X + (Local.X - Centre.X) / Pixels,
		Pan.Y - (Local.Y - Centre.Y) / Pixels);
}

FBox2D URollBallSkillGraphWidget::NodeBox(int32 NodeIndex, const FVector2D& CanvasSize) const
{
	if (Graph == nullptr || !Graph->IsValidNodeIndex(NodeIndex))
	{
		return FBox2D(FVector2D::ZeroVector, FVector2D::ZeroVector);
	}

	const FRollBallSkillNode& Node = Graph->GetNode(NodeIndex);

	const FVector2D Size(
		NodeBaseSize * Zoom * Node.Definition.ScaleX,
		NodeBaseSize * Zoom * Node.Definition.ScaleY);

	const FVector2D Centre = CellToLocal(Node.Cell, CanvasSize);
	return FBox2D(Centre - Size * 0.5, Centre + Size * 0.5);
}

int32 URollBallSkillGraphWidget::HitTestNode(const FVector2D& Local, const FVector2D& CanvasSize) const
{
	if (Graph == nullptr)
	{
		return INDEX_NONE;
	}

	for (int32 i = Graph->GetNodes().Num() - 1; i >= 0; --i)
	{
		if (NodeBox(i, CanvasSize).IsInside(Local))
		{
			return i;
		}
	}
	return INDEX_NONE;
}

bool URollBallSkillGraphWidget::ResolveBend(const FVector2D& A, const FVector2D& B,
	ERollBallEdgeRoute Route, FVector2D& OutBend)
{
	OutBend = FVector2D::ZeroVector;

	if (Route == ERollBallEdgeRoute::Straight
		|| FMath::IsNearlyEqual(A.X, B.X, 0.01)
		|| FMath::IsNearlyEqual(A.Y, B.Y, 0.01))
	{
		return false;
	}

	OutBend = (Route == ERollBallEdgeRoute::VerticalFirst)
		? FVector2D(A.X, B.Y)
		: FVector2D(B.X, A.Y);

	return true;
}

void URollBallSkillGraphWidget::FrameAll()
{
	if (Graph == nullptr || !Graph->IsBuilt())
	{
		return;
	}

	FVector2D Min;
	FVector2D Max;
	Graph->GetCellBounds(Min, Max);

	Pan = (Min + Max) * 0.5;

	const double Span = FMath::Max(2.0, FMath::Max(Max.X - Min.X, Max.Y - Min.Y) + 3.0);
	const double View = FMath::Min(
		FMath::Max(1.0, LastCanvasSize.X),
		FMath::Max(1.0, LastCanvasSize.Y));

	Zoom = FMath::Clamp(static_cast<float>(View / (Span * CellSize)), MinZoom, MaxZoom);
	TargetZoom = Zoom;
	bFocusing = false;
}

void URollBallSkillGraphWidget::FocusOnCore()
{
	if (Graph == nullptr)
	{
		return;
	}

	const int32 CoreIndex = Graph->GetCoreNodeIndex();
	if (Graph->IsValidNodeIndex(CoreIndex))
	{
		Pan = Graph->GetNode(CoreIndex).Cell;
	}
}

void URollBallSkillGraphWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	LastCanvasSize = FVector2D(MyGeometry.GetLocalSize());

	if (bPendingFrameAll && LastCanvasSize.X > 1.0 && LastCanvasSize.Y > 1.0)
	{
		bPendingFrameAll = false;
		FrameAll();
	}

	if (!FMath::IsNearlyEqual(Zoom, TargetZoom, 0.0005f))
	{

		const float Alpha = 1.0f - FMath::Exp(-ZoomDampRate * InDeltaTime);
		Zoom = FMath::Lerp(Zoom, TargetZoom, Alpha);
	}
	else if (!FMath::IsNearlyEqual(Zoom, TargetZoom))
	{
		Zoom = TargetZoom;
	}

	if (bFocusing)
	{

		Pan += FocusCell - LocalToCell(FocusLocal, LastCanvasSize);

		if (FMath::IsNearlyEqual(Zoom, TargetZoom, 0.0005f))
		{
			bFocusing = false;
		}
	}
}

int32 URollBallSkillGraphWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2D CanvasSize = FVector2D(AllottedGeometry.GetLocalSize());

	PaintBox(OutDrawElements, LayerId, AllottedGeometry, FVector2D::ZeroVector, CanvasSize, BackgroundColor);

	PaintGrid(AllottedGeometry, OutDrawElements, LayerId + 1);
	PaintEdges(AllottedGeometry, OutDrawElements, LayerId + 2);
	PaintNodes(AllottedGeometry, OutDrawElements, LayerId + 3);
	PaintTooltip(AllottedGeometry, OutDrawElements, LayerId + 6);

	return Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements,
		LayerId + 8, InWidgetStyle, bParentEnabled);
}

void URollBallSkillGraphWidget::PaintGrid(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 Layer) const
{
	const float Pixels = PixelsPerCell();
	if (Pixels < GridMinPixelSpacing)
	{

		return;
	}

	const FVector2D CanvasSize = FVector2D(Geometry.GetLocalSize());

	const FVector2D TopLeft = LocalToCell(FVector2D::ZeroVector, CanvasSize);
	const FVector2D BottomRight = LocalToCell(CanvasSize, CanvasSize);

	const int32 FirstX = FMath::FloorToInt(TopLeft.X);
	const int32 LastX = FMath::CeilToInt(BottomRight.X);
	const int32 FirstY = FMath::FloorToInt(BottomRight.Y);
	const int32 LastY = FMath::CeilToInt(TopLeft.Y);

	for (int32 X = FirstX; X <= LastX; ++X)
	{
		const double LocalX = CellToLocal(FVector2D(X, 0.0), CanvasSize).X;
		PaintLine(Elements, Layer, Geometry,
			FVector2D(LocalX, 0.0), FVector2D(LocalX, CanvasSize.Y), GridColor, 1.0f);
	}

	for (int32 Y = FirstY; Y <= LastY; ++Y)
	{
		const double LocalY = CellToLocal(FVector2D(0.0, Y), CanvasSize).Y;
		PaintLine(Elements, Layer, Geometry,
			FVector2D(0.0, LocalY), FVector2D(CanvasSize.X, LocalY), GridColor, 1.0f);
	}
}

void URollBallSkillGraphWidget::PaintEdges(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 Layer) const
{
	if (Graph == nullptr)
	{
		return;
	}

	const FVector2D CanvasSize = FVector2D(Geometry.GetLocalSize());

	for (const FRollBallSkillEdge& Edge : Graph->GetEdges())
	{
		const FVector2D From = CellToLocal(Graph->GetNode(Edge.NodeIndexA).Cell, CanvasSize);
		const FVector2D To = CellToLocal(Graph->GetNode(Edge.NodeIndexB).Cell, CanvasSize);

		const bool bBothOwned = Graph->IsOwned(Edge.NodeIndexA) && Graph->IsOwned(Edge.NodeIndexB);
		const bool bOnPath = PendingPath.Contains(Edge.NodeIndexA) || PendingPath.Contains(Edge.NodeIndexB);

		FLinearColor Color = EdgeColor;
		float Thickness = 2.0f;

		if (bBothOwned)
		{
			Color = EdgeOwnedColor;
			Thickness = 3.0f;
		}
		else if (bOnPath)
		{
			Color = EdgePathColor;
			Thickness = 3.0f;
		}

		FVector2D Bend;
		if (ResolveBend(From, To, Edge.RouteType, Bend))
		{
			PaintLine(Elements, Layer, Geometry, From, Bend, Color, Thickness);
			PaintLine(Elements, Layer, Geometry, Bend, To, Color, Thickness);
		}
		else
		{
			PaintLine(Elements, Layer, Geometry, From, To, Color, Thickness);
		}
	}
}

void URollBallSkillGraphWidget::PaintNodes(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 Layer) const
{
	if (Graph == nullptr)
	{
		return;
	}

	const FVector2D CanvasSize = FVector2D(Geometry.GetLocalSize());
	const int32 TextLayer = Layer + 1;

	for (int32 i = 0; i < Graph->GetNodes().Num(); ++i)
	{
		const FBox2D Box = NodeBox(i, CanvasSize);

		if (Box.Max.X < 0.0 || Box.Min.X > CanvasSize.X || Box.Max.Y < 0.0 || Box.Min.Y > CanvasSize.Y)
		{
			continue;
		}

		const FVector2D Size = Box.Max - Box.Min;
		const float BorderWidth = FMath::Max(1.5f, 2.5f * Zoom);

		PaintBox(Elements, Layer, Geometry, Box.Min, Size, NodeBorderColorOf(i));
		PaintBox(Elements, Layer, Geometry,
			Box.Min + FVector2D(BorderWidth, BorderWidth),
			Size - FVector2D(BorderWidth, BorderWidth) * 2.0,
			NodeFillColorOf(i));

		if (Size.Y < LabelMinPixelHeight)
		{
			continue;
		}

		const FRollBallSkillNode& Node = Graph->GetNode(i);

		FString Label = Node.Definition.DisplayName.ToString().Left(2);
		if (Label.IsEmpty())
		{
			Label = TEXT("?");
		}

		FSlateFontInfo Font = LabelFont;
		Font.Size = FMath::Clamp(FMath::RoundToInt(Size.Y * 0.42f), 6, 18);

		const FVector2D LabelSize = MeasureText(Label, Font);
		const FVector2D LabelPos = Box.Min + (Size - LabelSize) * 0.5;

		PaintText(Elements, TextLayer, Geometry, LabelPos, Label, Font,
			Graph->IsOwned(i) ? FLinearColor::White : FLinearColor(0.75f, 0.78f, 0.85f));

		if (Node.Definition.MaxLevel > 1 && Size.Y >= LabelMinPixelHeight * 1.6f)
		{
			FSlateFontInfo SmallFont = LabelFont;
			SmallFont.Size = FMath::Clamp(FMath::RoundToInt(Size.Y * 0.26f), 6, 12);

			const FString Rank = FString::Printf(TEXT("%d/%d"),
				Graph->GetLevel(i), Node.Definition.MaxLevel);

			const FVector2D RankSize = MeasureText(Rank, SmallFont);

			PaintText(Elements, TextLayer, Geometry,
				FVector2D(Box.Max.X - RankSize.X - 2.0, Box.Max.Y - RankSize.Y - 1.0),
				Rank, SmallFont, FLinearColor(1.0f, 0.92f, 0.55f));
		}
	}
}

void URollBallSkillGraphWidget::PaintTooltip(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 Layer) const
{
	if (Graph == nullptr || !Graph->IsValidNodeIndex(HoveredNodeIndex) || !bMouseInside)
	{
		return;
	}

	const FRollBallSkillNode& Node = Graph->GetNode(HoveredNodeIndex);
	const int32 Level = Graph->GetLevel(HoveredNodeIndex);
	const int32 Cost = Graph->GetNextCost(HoveredNodeIndex);
	const int32 Gold = GetGold();

	TArray<FString> Lines;
	Lines.Add(Node.Definition.DisplayName.ToString().ToUpper());

	if (!Node.Definition.Description.IsEmpty())
	{
		Lines.Add(Node.Definition.Description.ToString());
	}
	else if (Node.Definition.GrantStat != ERollBallSkillStat::None)
	{
		Lines.Add(FRollBallSkillStats::DescribeAmount(
			Node.Definition.GrantStat, Node.Definition.AmountPerLevel).ToString());
	}

	Lines.Add(FString::Printf(TEXT("level: %d/%d"), Level, Node.Definition.MaxLevel));

	if (Node.Definition.NodeKind == ERollBallSkillNodeKind::Gate
		&& !Graph->IsGateOpen(HoveredNodeIndex, GetBestStage()))
	{
		Lines.Add(FString::Printf(TEXT("잠김 - 최고 %d 스테이지 필요"), Node.Definition.RequiredBestStage));
	}
	else if (Cost > 0)
	{
		Lines.Add(FString::Printf(TEXT("$  %d/%d"), Gold, Cost));
	}
	else
	{
		Lines.Add(TEXT("만렙"));
	}

	if (HoveredNodeIndex == SelectedNodeIndex && PendingPath.Num() > 1)
	{
		Lines.Add(FString::Printf(TEXT("경로 %d칸 / 합계 %d"),
			PendingPath.Num(), Graph->GetPathCost(PendingPath)));
	}

	const double PanelPadding = 10.0;
	const double RowHeight = TextLineHeight(TooltipFont) + 2.0;

	double Width = 0.0;
	for (const FString& Row : Lines)
	{
		Width = FMath::Max(Width, MeasureText(Row, TooltipFont).X);
	}

	const FVector2D PanelSize(Width + PanelPadding * 2.0, RowHeight * Lines.Num() + PanelPadding * 2.0);
	const FVector2D CanvasSize = FVector2D(Geometry.GetLocalSize());

	FVector2D PanelPos = MouseLocal + FVector2D(16.0, 16.0);
	PanelPos.X = FMath::Clamp(PanelPos.X, 0.0, FMath::Max(0.0, CanvasSize.X - PanelSize.X));
	PanelPos.Y = FMath::Clamp(PanelPos.Y, 0.0, FMath::Max(0.0, CanvasSize.Y - PanelSize.Y));

	PaintBox(Elements, Layer, Geometry, PanelPos, PanelSize, TooltipBackColor);
	PaintBox(Elements, Layer, Geometry, PanelPos, FVector2D(PanelSize.X, 2.0),
		BranchColorOf(Node.Definition.Branch));

	for (int32 i = 0; i < Lines.Num(); ++i)
	{
		const FLinearColor Color = (i == 0)
			? FLinearColor::White
			: FLinearColor(0.72f, 0.78f, 0.88f);

		PaintText(Elements, Layer + 1, Geometry,
			PanelPos + FVector2D(PanelPadding, PanelPadding + RowHeight * i),
			Lines[i], TooltipFont, Color);
	}
}

FLinearColor URollBallSkillGraphWidget::BranchColorOf(ERollBallSkillBranch Branch) const
{
	switch (Branch)
	{
	case ERollBallSkillBranch::Core:     return FLinearColor(0.85f, 0.85f, 0.90f);
	case ERollBallSkillBranch::Survival: return FLinearColor(0.85f, 0.28f, 0.33f);
	case ERollBallSkillBranch::Mobility: return FLinearColor(0.26f, 0.62f, 0.92f);
	case ERollBallSkillBranch::Weapon:   return FLinearColor(0.78f, 0.45f, 0.92f);
	case ERollBallSkillBranch::Economy:  return FLinearColor(0.95f, 0.75f, 0.25f);
	default:                             return FLinearColor::Gray;
	}
}

FLinearColor URollBallSkillGraphWidget::NodeFillColorOf(int32 NodeIndex) const
{

	const FRollBallSkillNode& Node = Graph->GetNode(NodeIndex);
	FLinearColor Color = BranchColorOf(Node.Definition.Branch);

	if (!Graph->IsOwned(NodeIndex))
	{
		const bool bReachable = Graph->HasOwnedNeighbour(NodeIndex)
			&& Graph->IsGateOpen(NodeIndex, GetBestStage());

		Color *= bReachable ? 0.62f : 0.28f;
		Color.A = 1.0f;
	}

	return Color;
}

FLinearColor URollBallSkillGraphWidget::NodeBorderColorOf(int32 NodeIndex) const
{
	if (NodeIndex == SelectedNodeIndex)
	{
		return SelectedBorderColor;
	}

	if (Graph->IsOwned(NodeIndex))
	{
		return OwnedBorderColor;
	}

	FText Reason;
	if (Graph->CanBuy(NodeIndex, GetGold(), GetBestStage(), Reason))
	{
		return BuyableBorderColor;
	}

	return LockedBorderColor;
}

FReply URollBallSkillGraphWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const FVector2D Local = FVector2D(InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()));
	const FVector2D CanvasSize = FVector2D(InGeometry.GetLocalSize());

	if (InMouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton
		|| InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		bPanning = true;
		bFocusing = false;
		LastPanLocal = Local;
		return FReply::Handled().CaptureMouse(TakeWidget());
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const int32 Hit = HitTestNode(Local, CanvasSize);

		if (Hit == INDEX_NONE)
		{
			SelectNode(INDEX_NONE);
		}
		else if (Hit == SelectedNodeIndex)
		{

			AttemptPurchase(Hit);
		}
		else
		{
			SelectNode(Hit);
		}

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply URollBallSkillGraphWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bPanning
		&& (InMouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton
			|| InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton))
	{
		bPanning = false;
		return FReply::Handled().ReleaseMouseCapture();
	}

	return FReply::Unhandled();
}

FReply URollBallSkillGraphWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const FVector2D Local = FVector2D(InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()));
	const FVector2D CanvasSize = FVector2D(InGeometry.GetLocalSize());

	MouseLocal = Local;
	bMouseInside = true;

	if (bPanning)
	{

		const FVector2D Before = LocalToCell(LastPanLocal, CanvasSize);
		const FVector2D Now = LocalToCell(Local, CanvasSize);
		Pan += Before - Now;

		LastPanLocal = Local;
		return FReply::Handled();
	}

	HoveredNodeIndex = HitTestNode(Local, CanvasSize);
	return FReply::Handled();
}

FReply URollBallSkillGraphWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const FVector2D Local = FVector2D(InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()));
	const FVector2D CanvasSize = FVector2D(InGeometry.GetLocalSize());

	TargetZoom = FMath::Clamp(
		TargetZoom * FMath::Exp(InMouseEvent.GetWheelDelta() * ZoomStep),
		MinZoom, MaxZoom);

	FocusLocal = Local;
	FocusCell = LocalToCell(Local, CanvasSize);
	bFocusing = true;

	return FReply::Handled();
}

void URollBallSkillGraphWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	bMouseInside = false;
	HoveredNodeIndex = INDEX_NONE;
}

FReply URollBallSkillGraphWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		RemoveFromParent();
		return FReply::Handled();
	}

	if (InKeyEvent.GetKey() == EKeys::Home)
	{
		FrameAll();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void URollBallSkillGraphWidget::SelectNode(int32 NodeIndex)
{
	SelectedNodeIndex = NodeIndex;
	RefreshPendingPath();
}

void URollBallSkillGraphWidget::RefreshPendingPath()
{
	PendingPath.Reset();

	if (Graph == nullptr || !Graph->IsValidNodeIndex(SelectedNodeIndex))
	{
		return;
	}

	Graph->FindPathTo(SelectedNodeIndex, GetBestStage(), PendingPath);
}

void URollBallSkillGraphWidget::AttemptPurchase(int32 NodeIndex)
{
	URollBallGameInstance* GameInstance = GetRollBallGameInstance();
	if (Graph == nullptr || GameInstance == nullptr)
	{
		return;
	}

	const int32 GoldBefore = GetGold();
	FText Reason;

	if (Graph->CanBuy(NodeIndex, GoldBefore, GetBestStage(), Reason))
	{
		if (GameInstance->BuySkillNode(NodeIndex, Reason))
		{
			OnPurchased(NodeIndex, GoldBefore - GetGold());
			RefreshPendingPath();
			return;
		}
	}

	else if (PendingPath.Num() > 0)
	{
		if (GameInstance->BuySkillPath(PendingPath, Reason))
		{
			OnPurchased(NodeIndex, GoldBefore - GetGold());
			RefreshPendingPath();
			return;
		}
	}

	OnPurchaseRefused(Reason);
}

URollBallGameInstance* URollBallSkillGraphWidget::GetRollBallGameInstance() const
{
	return GetGameInstance<URollBallGameInstance>();
}

int32 URollBallSkillGraphWidget::GetGold() const
{
	const URollBallGameInstance* GameInstance = GetRollBallGameInstance();
	return GameInstance != nullptr ? GameInstance->GetGold() : 0;
}

int32 URollBallSkillGraphWidget::GetBestStage() const
{
	const URollBallGameInstance* GameInstance = GetRollBallGameInstance();
	return GameInstance != nullptr ? GameInstance->GetBestStage() : 0;
}

#undef LOCTEXT_NAMESPACE
