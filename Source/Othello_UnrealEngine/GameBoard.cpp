// Fill out your copyright notice in the Description page of Project Settings.

#include "GameBoard.h"

const int32 AGameBoard::Directions[8][2] = {
	{-1, -1}, {-1, 0}, {-1, 1},
	{ 0, -1},           { 0, 1},
	{ 1, -1}, { 1, 0}, { 1, 1}
};

AGameBoard::AGameBoard()
{
	PrimaryActorTick.bCanEverTick = false;

	Board.Init(ECellState::Empty, BoardSize * BoardSize);
}

void AGameBoard::BeginPlay()
{
	Super::BeginPlay();
	InitializeBoard();
}

void AGameBoard::InitializeBoard()
{
	Board.Init(ECellState::Empty, BoardSize * BoardSize);

	// Standard Othello starting position:
	//   W B
	//   B W  centered on the board
	const int32 Mid = BoardSize / 2;
	Board[(Mid - 1) * BoardSize + (Mid - 1)] = ECellState::White;
	Board[(Mid - 1) * BoardSize +  Mid     ] = ECellState::Black;
	Board[ Mid      * BoardSize + (Mid - 1)] = ECellState::Black;
	Board[ Mid      * BoardSize +  Mid     ] = ECellState::White;
}

ECellState AGameBoard::GetCell(int32 Row, int32 Col) const
{
	if (!InBounds(Row, Col))
	{
		return ECellState::Empty;
	}
	return Board[Row * BoardSize + Col];
}

bool AGameBoard::InBounds(int32 Row, int32 Col) const
{
	return Row >= 0 && Row < BoardSize && Col >= 0 && Col < BoardSize;
}

bool AGameBoard::GetFlipsInDirection(int32 Row, int32 Col, int32 DRow, int32 DCol,
                                     ECellState PlayerState, TArray<int32>& OutFlips) const
{
	const ECellState OpponentState = (PlayerState == ECellState::Black)
		? ECellState::White
		: ECellState::Black;

	TArray<int32> Candidates;
	int32 R = Row + DRow;
	int32 C = Col + DCol;

	// Walk in this direction, collecting opponent pieces.
	while (InBounds(R, C) && Board[R * BoardSize + C] == OpponentState)
	{
		Candidates.Add(R * BoardSize + C);
		R += DRow;
		C += DCol;
	}

	// Valid only if we ended on one of our own pieces (and collected at least one opponent).
	if (Candidates.Num() > 0 && InBounds(R, C) && Board[R * BoardSize + C] == PlayerState)
	{
		OutFlips.Append(Candidates);
		return true;
	}

	return false;
}

bool AGameBoard::IsValidMove(int32 Row, int32 Col, bool bIsBlackTurn) const
{
	if (!InBounds(Row, Col) || Board[Row * BoardSize + Col] != ECellState::Empty)
	{
		return false;
	}

	const ECellState PlayerState = bIsBlackTurn ? ECellState::Black : ECellState::White;

	for (const auto& Dir : Directions)
	{
		TArray<int32> Flips;
		if (GetFlipsInDirection(Row, Col, Dir[0], Dir[1], PlayerState, Flips))
		{
			return true;
		}
	}

	return false;
}

TArray<int32> AGameBoard::GetValidMoves(bool bIsBlackTurn) const
{
	TArray<int32> ValidMoves;

	for (int32 Row = 0; Row < BoardSize; ++Row)
	{
		for (int32 Col = 0; Col < BoardSize; ++Col)
		{
			if (IsValidMove(Row, Col, bIsBlackTurn))
			{
				ValidMoves.Add(Row * BoardSize + Col);
			}
		}
	}

	return ValidMoves;
}

bool AGameBoard::PlacePiece(int32 Row, int32 Col, bool bIsBlackTurn)
{
	if (!IsValidMove(Row, Col, bIsBlackTurn))
	{
		return false;
	}

	const ECellState PlayerState = bIsBlackTurn ? ECellState::Black : ECellState::White;

	// Collect all pieces to flip across every direction.
	TArray<int32> AllFlips;
	for (const auto& Dir : Directions)
	{
		GetFlipsInDirection(Row, Col, Dir[0], Dir[1], PlayerState, AllFlips);
	}

	// Place the new piece.
	Board[Row * BoardSize + Col] = PlayerState;

	// Flip captured pieces.
	for (int32 Index : AllFlips)
	{
		Board[Index] = PlayerState;
	}

	return true;
}

bool AGameBoard::IsGameOver() const
{
	return GetValidMoves(true).Num() == 0 && GetValidMoves(false).Num() == 0;
}

int32 AGameBoard::GetScore(bool bIsBlack) const
{
	const ECellState Target = bIsBlack ? ECellState::Black : ECellState::White;
	int32 Count = 0;

	for (const ECellState& Cell : Board)
	{
		if (Cell == Target)
		{
			++Count;
		}
	}

	return Count;
}
