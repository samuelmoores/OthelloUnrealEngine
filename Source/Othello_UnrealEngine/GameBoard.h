// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameBoard.generated.h"

UENUM(BlueprintType)
enum class ECellState : uint8
{
	Empty  UMETA(DisplayName = "Empty"),
	Black  UMETA(DisplayName = "Black"),
	White  UMETA(DisplayName = "White"),
};

UCLASS(Blueprintable)
class OTHELLO_UNREALENGINE_API AGameBoard : public AActor
{
	GENERATED_BODY()

public:
	AGameBoard();

	static const int32 BoardSize = 8;

	// Flat 8x8 board. Index via Row * BoardSize + Col.
	UPROPERTY(BlueprintReadOnly, Category = "Othello|Board")
	TArray<ECellState> Board;

	// Initializes the board to the standard Othello starting position.
	UFUNCTION(BlueprintCallable, Category = "Othello|Board")
	void InitializeBoard();

	// Returns true if placing a piece at (Row, Col) is a legal move for the given player.
	UFUNCTION(BlueprintCallable, Category = "Othello|Board")
	bool IsValidMove(int32 Row, int32 Col, bool bIsBlackTurn) const;

	// Returns all valid move indices (Row * BoardSize + Col) for the given player.
	UFUNCTION(BlueprintCallable, Category = "Othello|Board")
	TArray<int32> GetValidMoves(bool bIsBlackTurn) const;

	// Places a piece at (Row, Col) for the given player and flips captured pieces.
	// Returns false if the move is not valid.
	UFUNCTION(BlueprintCallable, Category = "Othello|Board")
	bool PlacePiece(int32 Row, int32 Col, bool bIsBlackTurn);

	// Returns true when neither player has any valid moves.
	UFUNCTION(BlueprintCallable, Category = "Othello|Board")
	bool IsGameOver() const;

	// Returns the number of pieces on the board for the given player.
	UFUNCTION(BlueprintCallable, Category = "Othello|Board")
	int32 GetScore(bool bIsBlack) const;

	// Returns the cell state at (Row, Col).
	UFUNCTION(BlueprintCallable, Category = "Othello|Board")
	ECellState GetCell(int32 Row, int32 Col) const;

protected:
	virtual void BeginPlay() override;

private:
	// The 8 cardinal and diagonal directions.
	static const int32 Directions[8][2];

	// Returns true if (Row, Col) is within the board bounds.
	bool InBounds(int32 Row, int32 Col) const;

	// Returns the pieces that would be flipped in a single direction if a piece
	// is placed at (Row, Col). Populates OutFlips with their flat indices.
	bool GetFlipsInDirection(int32 Row, int32 Col, int32 DRow, int32 DCol,
	                         ECellState PlayerState, TArray<int32>& OutFlips) const;
};
