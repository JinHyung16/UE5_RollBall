#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RollBall/Skill/RollBallSkillTypes.h"
#include "RollBallSkillGraphWidget.generated.h"

class URollBallSkillGraph;
class URollBallGameInstance;

UCLASS()
class ROLLBALL_API URollBallSkillGraphWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	URollBallSkillGraphWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "RollBall|Skill")
	void SetGraph(URollBallSkillGraph* InGraph);

	UFUNCTION(BlueprintCallable, Category = "RollBall|Skill")
	void FrameAll();

	UFUNCTION(BlueprintCallable, Category = "RollBall|Skill")
	void FocusOnCore();

	UFUNCTION(BlueprintImplementableEvent, Category = "RollBall|Skill")
	void OnPurchaseRefused(const FText& Reason);

	UFUNCTION(BlueprintImplementableEvent, Category = "RollBall|Skill")
	void OnPurchased(int32 NodeIndex, int32 SpentGold);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "격자", meta = (ClampMin = "8"))
	float CellSize = 56.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "격자", meta = (ClampMin = "4"))
	float NodeBaseSize = 34.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "격자", meta = (ClampMin = "0.05"))
	float MinZoom = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "격자", meta = (ClampMin = "0.1"))
	float MaxZoom = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "격자")
	float ZoomStep = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "격자")
	float ZoomDampRate = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "격자")
	float LabelMinPixelHeight = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "격자")
	float GridMinPixelSpacing = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "색")
	FLinearColor BackgroundColor = FLinearColor(0.043f, 0.047f, 0.063f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "색")
	FLinearColor GridColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.045f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "색")
	FLinearColor EdgeColor = FLinearColor(0.32f, 0.36f, 0.45f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "색")
	FLinearColor EdgeOwnedColor = FLinearColor(0.55f, 0.95f, 0.62f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "색")
	FLinearColor EdgePathColor = FLinearColor(1.0f, 0.85f, 0.35f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "색")
	FLinearColor OwnedBorderColor = FLinearColor(0.42f, 1.0f, 0.5f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "색")
	FLinearColor BuyableBorderColor = FLinearColor(0.95f, 0.95f, 0.98f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "색")
	FLinearColor LockedBorderColor = FLinearColor(0.22f, 0.24f, 0.30f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "색")
	FLinearColor SelectedBorderColor = FLinearColor(1.0f, 0.85f, 0.35f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "색")
	FLinearColor TooltipBackColor = FLinearColor(0.02f, 0.02f, 0.03f, 0.94f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "색")
	FSlateFontInfo LabelFont;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "색")
	FSlateFontInfo TooltipFont;

private:
	UPROPERTY()
	TObjectPtr<URollBallSkillGraph> Graph = nullptr;

	FVector2D Pan = FVector2D::ZeroVector;

	float Zoom = 1.0f;
	float TargetZoom = 1.0f;

	FVector2D FocusLocal = FVector2D::ZeroVector;
	FVector2D FocusCell = FVector2D::ZeroVector;
	bool bFocusing = false;

	bool bPanning = false;
	FVector2D LastPanLocal = FVector2D::ZeroVector;

	FVector2D MouseLocal = FVector2D::ZeroVector;
	bool bMouseInside = false;

	int32 SelectedNodeIndex = INDEX_NONE;
	int32 HoveredNodeIndex = INDEX_NONE;

	TArray<int32> PendingPath;

	FVector2D LastCanvasSize = FVector2D(1024.0, 768.0);

	bool bPendingFrameAll = false;

	float PixelsPerCell() const { return CellSize * Zoom; }

	FVector2D CellToLocal(const FVector2D& Cell, const FVector2D& CanvasSize) const;
	FVector2D LocalToCell(const FVector2D& Local, const FVector2D& CanvasSize) const;

	FBox2D NodeBox(int32 NodeIndex, const FVector2D& CanvasSize) const;

	int32 HitTestNode(const FVector2D& Local, const FVector2D& CanvasSize) const;

	static bool ResolveBend(const FVector2D& A, const FVector2D& B, ERollBallEdgeRoute Route, FVector2D& OutBend);

	void PaintGrid(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 Layer) const;
	void PaintEdges(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 Layer) const;
	void PaintNodes(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 Layer) const;
	void PaintTooltip(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 Layer) const;

	FLinearColor BranchColorOf(ERollBallSkillBranch Branch) const;
	FLinearColor NodeFillColorOf(int32 NodeIndex) const;
	FLinearColor NodeBorderColorOf(int32 NodeIndex) const;

	void SelectNode(int32 NodeIndex);
	void AttemptPurchase(int32 NodeIndex);
	void RefreshPendingPath();

	URollBallGameInstance* GetRollBallGameInstance() const;
	int32 GetGold() const;
	int32 GetBestStage() const;

	UFUNCTION()
	void HandleGraphChanged();
};
