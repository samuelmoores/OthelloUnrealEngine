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

	InitializeBoard();
}

void AGameBoard::BeginPlay()
{
	Super::BeginPlay();
	InitializeBoard();

	TArray<int32> ValidMoves = GetValidMoves();
	FString MovesString = TEXT("Valid moves:");
	for (int32 Square : ValidMoves)
	{
		MovesString += FString::Printf(TEXT(" %d"), Square);
	}
	UE_LOG(LogTemp, Log, TEXT("%s"), bIsBlackTurn ? TEXT("Black's turn") : TEXT("White's turn"));
	UE_LOG(LogTemp, Log, TEXT("%s"), *MovesString);
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

	bIsBlackTurn = true;

	UE_LOG(LogTemp, Log, TEXT("InitializeBoard complete. Center cells set to starting position."));
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

TArray<int32> AGameBoard::GetValidMoves() const
{
	TArray<int32> ValidMoves;

	for (int32 Row = 0; Row < BoardSize; ++Row)
	{
		for (int32 Col = 0; Col < BoardSize; ++Col)
		{
			if (IsValidMove(Row, Col))
			{
				ValidMoves.Add(Row * BoardSize + Col);
			}
		}
	}

	return ValidMoves;
}

bool AGameBoard::PlacePiece(int32 Row, int32 Col)
{
	if (!IsValidMove(Row, Col))
	{
		return false;
	}

	const ECellState PieceState = bIsBlackTurn ? ECellState::Black : ECellState::White;

	LastFlips.Reset();
	for (const auto& Dir : Directions)
	{
		GetFlipsInDirection(Row, Col, Dir[0], Dir[1], PieceState, LastFlips);
	}

	Board[Row * BoardSize + Col] = PieceState;

	for (int32 Index : LastFlips)
	{
		Board[Index] = PieceState;
	}

	return true;
}

bool AGameBoard::HasAnyValidMove(bool bForBlack) const
{
	const ECellState PieceState = bForBlack ? ECellState::Black : ECellState::White;

	for (int32 Row = 0; Row < BoardSize; ++Row)
	{
		for (int32 Col = 0; Col < BoardSize; ++Col)
		{
			if (Board[Row * BoardSize + Col] != ECellState::Empty)
			{
				continue;
			}

			for (const auto& Dir : Directions)
			{
				TArray<int32> Flips;
				if (GetFlipsInDirection(Row, Col, Dir[0], Dir[1], PieceState, Flips))
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool AGameBoard::IsGameOver() const
{
	return !HasAnyValidMove(true) && !HasAnyValidMove(false);
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

TArray<int32> AGameBoard::GetScores() const
{
	return { GetScore(false), GetScore(true) };
}

bool AGameBoard::IsValidMoveForSquare(const FString& Input) const
{
	int32 Square;
	if (!ParseSquareInput(Input, Square))
	{
		return false;
	}
	return IsValidMove(Square / BoardSize, Square % BoardSize);
}

bool AGameBoard::ParseSquareInput(const FString& Input, int32& OutIndex) const
{
	if (Input.Len() != 2)
	{
		return false;
	}

	if (FChar::IsDigit(Input[0]) && FChar::IsDigit(Input[1]))
	{
		OutIndex = FCString::Atoi(*Input) - 1;
	}
	else if (!FChar::IsDigit(Input[0]) && FChar::IsDigit(Input[1]))
	{
		OutIndex = FChar::ConvertCharDigitToInt(Input[1]) - 1;
	}
	else
	{
		return false;
	}

	return OutIndex >= 0 && OutIndex < BoardSize * BoardSize;
}

bool AGameBoard::IsValidMove(int32 Row, int32 Col) const
{
	if (!InBounds(Row, Col) || Board[Row * BoardSize + Col] != ECellState::Empty)
	{
		return false;
	}

	const ECellState PieceState = bIsBlackTurn ? ECellState::Black : ECellState::White;

	for (const auto& Dir : Directions)
	{
		TArray<int32> Flips;
		if (GetFlipsInDirection(Row, Col, Dir[0], Dir[1], PieceState, Flips))
		{
			return true;
		}
	}

	return false;
}

bool AGameBoard::GetFlipsInDirection(int32 Row, int32 Col, int32 DRow, int32 DCol,
	ECellState PieceState, TArray<int32>& OutFlips) const
{
	const ECellState OpponentState = (PieceState == ECellState::Black)
		? ECellState::White
		: ECellState::Black;

	TArray<int32> Candidates;
	int32 R = Row + DRow;
	int32 C = Col + DCol;

	while (InBounds(R, C) && Board[R * BoardSize + C] == OpponentState)
	{
		Candidates.Add(R * BoardSize + C);
		R += DRow;
		C += DCol;
	}

	if (Candidates.Num() > 0 && InBounds(R, C) && Board[R * BoardSize + C] == PieceState)
	{
		OutFlips.Append(Candidates);
		return true;
	}

	return false;
}


bool AGameBoard::HasPieceAtSquare(const FString& Input) const
{
	int32 Square;
	if (!ParseSquareInput(Input, Square))
	{
		return false;
	}
	return Board[Square] != ECellState::Empty;
}

ECellState AGameBoard::GetStateAtSquare(const FString& Input) const
{
	int32 Square;
	if (!ParseSquareInput(Input, Square))
	{
		return ECellState::Empty;
	}
	return Board[Square];
}

bool AGameBoard::ShouldFlipSquare(const FString& Input) const
{
	int32 Square;
	if (!ParseSquareInput(Input, Square))
	{
		return false;
	}
	return LastFlips.Contains(Square);
}

FString AGameBoard::GetRandomAIMove()
{
	TArray<int32> ValidMoves = GetValidMoves();
	if (ValidMoves.Num() == 0) return TEXT("");

	int32 Square = ValidMoves[FMath::RandRange(0, ValidMoves.Num() - 1)];
	return FString::Printf(TEXT("%02d"), Square + 1);
}

int AGameBoard::CurrentPlayer()
{
	if (!bIsBlackTurn)
		return 0;
	else
		return 1;
}

int32 AGameBoard::ApplyMove(const FString& Input)
{
	int32 Square;
	if (!ParseSquareInput(Input, Square))
	{
		return -1;
	}

	if (!PlacePiece(Square / BoardSize, Square % BoardSize))
	{
		return -1;
	}

	bIsBlackTurn = !bIsBlackTurn;
	return Square;
}
