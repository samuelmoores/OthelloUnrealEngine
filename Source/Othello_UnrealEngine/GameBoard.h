// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameBoard.generated.h"

UENUM(BlueprintType)
enum class ECellState : uint8
{
	Empty,
	Black,
	White,
};

UCLASS()
class OTHELLO_UNREALENGINE_API AGameBoard : public APawn
{
	GENERATED_BODY()

public:
	AGameBoard();

	static const int32 BoardSize = 8;
	TArray<ECellState> Board;
	bool bIsBlackTurn;

	void InitializeBoard();
	bool IsValidMove(int32 Row, int32 Col) const;
	TArray<int32> GetValidMoves() const;
	bool PlacePiece(int32 Row, int32 Col);
	bool IsGameOver() const;
	int32 GetScore(bool bIsBlack) const;
	ECellState GetCell(int32 Row, int32 Col) const;

	UFUNCTION(BlueprintCallable)
	int32 ApplyMove(const FString& Input);

	UFUNCTION(BlueprintCallable)
	bool IsValidMoveForSquare(const FString& Input) const;

	UFUNCTION(BlueprintCallable)
	bool HasPieceAtSquare(const FString& Input) const;

	UFUNCTION(BlueprintCallable)
	ECellState GetStateAtSquare(const FString& Input) const;

	UFUNCTION(BlueprintCallable)
	bool ShouldFlipSquare(const FString& Input) const;

	UFUNCTION(BlueprintCallable)
	int CurrentPlayer();

	UFUNCTION(BlueprintCallable)
	FString GetRandomAIMove();

	UFUNCTION(BlueprintCallable)
	TArray<int32> GetScores() const;

protected:
	virtual void BeginPlay() override;

private:
	static const int32 Directions[8][2];

	TArray<int32> LastFlips;

	bool InBounds(int32 Row, int32 Col) const;
	bool HasAnyValidMove(bool bForBlack) const;
	bool GetFlipsInDirection(int32 Row, int32 Col, int32 DRow, int32 DCol,
	                         ECellState PieceState, TArray<int32>& OutFlips) const;
	bool ParseSquareInput(const FString& Input, int32& OutIndex) const;
};
