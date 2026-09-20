#pragma once

#include "CoreMinimal.h"
#include "Brushes/SlateColorBrush.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace RollBallPaint
{

	inline const FSlateBrush& WhiteBoxBrush()
	{
		static const FSlateColorBrush Brush(FLinearColor::White);
		return Brush;
	}

	inline FSlateFontInfo PaintFont(int32 Size, bool bBold = false, int32 Outline = 1)
	{
		FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);

		Font.OutlineSettings.OutlineSize = Outline;
		Font.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.75f);

		return Font;
	}

	inline FVector2D MeasureText(const FString& Text, const FSlateFontInfo& InFont)
	{
		return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Text, InFont);
	}

	inline float TextLineHeight(const FSlateFontInfo& InFont)
	{
		return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->GetMaxCharacterHeight(InFont);
	}

	inline void PaintBox(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry,
		const FVector2D& Position, const FVector2D& Size, const FLinearColor& Color)
	{
		FSlateDrawElement::MakeBox(
			Elements,
			Layer,
			Geometry.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(Position))),
			&WhiteBoxBrush(),
			ESlateDrawEffect::None,
			Color);
	}

	inline void PaintPanel(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry,
		const FVector2D& Position, const FVector2D& Size,
		const FLinearColor& Fill, const FLinearColor& Border, float BorderWidth = 2.0f)
	{
		PaintBox(Elements, Layer, Geometry, Position, Size, Border);
		PaintBox(Elements, Layer, Geometry,
			Position + FVector2D(BorderWidth, BorderWidth),
			Size - FVector2D(BorderWidth, BorderWidth) * 2.0,
			Fill);
	}

	inline void PaintText(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry,
		const FVector2D& Position, const FString& InText, const FSlateFontInfo& InFont,
		const FLinearColor& Color)
	{
		FSlateDrawElement::MakeText(
			Elements,
			Layer,
			Geometry.ToPaintGeometry(Geometry.GetLocalSize(), FSlateLayoutTransform(FVector2f(Position))),
			InText,
			InFont,
			ESlateDrawEffect::None,
			Color);
	}

	inline void PaintTextCentered(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry,
		const FVector2D& BoxPosition, const FVector2D& BoxSize, const FString& InText,
		const FSlateFontInfo& InFont, const FLinearColor& Color)
	{
		const FVector2D Size = MeasureText(InText, InFont);
		PaintText(Elements, Layer, Geometry, BoxPosition + (BoxSize - Size) * 0.5, InText, InFont, Color);
	}

	inline void PaintLine(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry,
		const FVector2D& From, const FVector2D& To, const FLinearColor& Color, float Thickness)
	{
		TArray<FVector2D> Points;
		Points.Reserve(2);
		Points.Add(From);
		Points.Add(To);

		FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(),
			Points, ESlateDrawEffect::None, Color, true, Thickness);
	}

	inline FString FormatClock(float Seconds)
	{
		const int32 Total = FMath::Max(0, FMath::CeilToInt(Seconds));
		return FString::Printf(TEXT("%d:%02d"), Total / 60, Total % 60);
	}
}
