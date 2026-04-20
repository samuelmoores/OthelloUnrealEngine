// Fill out your copyright notice in the Description page of Project Settings.

#include "GameBoard.h"

const int32 AGameBoard::Directions[8][2] = {
	{-1, -1}, {-1, 0}, {-1, 1},
	{ 0, -1},           { 0, 1},
	{ 1, -1}, { 1, 0}, { 1, 1}
};

// ---------------------------------------------------------------------------
// Self-contained minimax worker — owns its own board copy, safe on any thread
// ---------------------------------------------------------------------------
namespace
{
static const int32 GDirs[8][2] = {
	{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}
};

struct FMinimaxSearch
{
	TArray<ECellState> Board;
	bool   bIsBlackTurn;
	double Deadline;
	bool   bTimedOut;

	FMinimaxSearch(const TArray<ECellState>& InBoard, bool bInTurn)
		: Board(InBoard), bIsBlackTurn(bInTurn), Deadline(0.0), bTimedOut(false) {}

	static bool InBounds(int32 R, int32 C)
	{
		return R >= 0 && R < 8 && C >= 0 && C < 8;
	}

	bool GetFlipsInDirection(int32 Row, int32 Col, int32 DR, int32 DC,
		ECellState Piece, TArray<int32>& Flips) const
	{
		const ECellState Opp = (Piece == ECellState::Black) ? ECellState::White : ECellState::Black;
		TArray<int32> Cands;
		int32 R = Row + DR, C = Col + DC;
		while (InBounds(R, C) && Board[R * 8 + C] == Opp)
		{
			Cands.Add(R * 8 + C);
			R += DR; C += DC;
		}
		if (Cands.Num() > 0 && InBounds(R, C) && Board[R * 8 + C] == Piece)
		{
			Flips.Append(Cands);
			return true;
		}
		return false;
	}

	bool IsValidMove(int32 Row, int32 Col) const
	{
		if (!InBounds(Row, Col) || Board[Row * 8 + Col] != ECellState::Empty) return false;
		const ECellState Piece = bIsBlackTurn ? ECellState::Black : ECellState::White;
		for (const auto& D : GDirs)
		{
			TArray<int32> F;
			if (GetFlipsInDirection(Row, Col, D[0], D[1], Piece, F)) return true;
		}
		return false;
	}

	TArray<int32> GetValidMoves() const
	{
		TArray<int32> Moves;
		for (int32 R = 0; R < 8; ++R)
			for (int32 C = 0; C < 8; ++C)
				if (IsValidMove(R, C)) Moves.Add(R * 8 + C);
		return Moves;
	}

	void PlacePiece(int32 Row, int32 Col)
	{
		const ECellState Piece = bIsBlackTurn ? ECellState::Black : ECellState::White;
		TArray<int32> Flips;
		for (const auto& D : GDirs)
			GetFlipsInDirection(Row, Col, D[0], D[1], Piece, Flips);
		Board[Row * 8 + Col] = Piece;
		for (int32 Idx : Flips) Board[Idx] = Piece;
	}

	bool HasAnyValidMove(bool bForBlack)
	{
		const bool bSaved = bIsBlackTurn;
		bIsBlackTurn = bForBlack;
		const bool bResult = GetValidMoves().Num() > 0;
		bIsBlackTurn = bSaved;
		return bResult;
	}

	bool IsGameOver()
	{
		return !HasAnyValidMove(true) && !HasAnyValidMove(false);
	}

	int32 EvaluateBoard() const
	{
		static const int32 Weights[64] = {
			100, -20,  10,  5,  5,  10, -20, 100,
			-20, -50,  -2, -2, -2,  -2, -50, -20,
			 10,  -2,   5,  1,  1,   5,  -2,  10,
			  5,  -2,   1,  0,  0,   1,  -2,   5,
			  5,  -2,   1,  0,  0,   1,  -2,   5,
			 10,  -2,   5,  1,  1,   5,  -2,  10,
			-20, -50,  -2, -2, -2,  -2, -50, -20,
			100, -20,  10,  5,  5,  10, -20, 100,
		};
		int32 Score = 0;
		for (int32 i = 0; i < 64; ++i)
		{
			if      (Board[i] == ECellState::Black) Score += Weights[i];
			else if (Board[i] == ECellState::White) Score -= Weights[i];
		}
		return Score;
	}

	int32 Minimax(int32 Depth, int32 Alpha, int32 Beta, bool bMaximizing)
	{
		if (FPlatformTime::Seconds() >= Deadline)
		{
			bTimedOut = true;
			return 0;
		}
		if (Depth == 0 || IsGameOver())
			return EvaluateBoard();

		TArray<int32> Moves = GetValidMoves();
		if (Moves.Num() == 0)
		{
			bIsBlackTurn = !bIsBlackTurn;
			int32 Result = Minimax(Depth - 1, Alpha, Beta, !bMaximizing);
			bIsBlackTurn = !bIsBlackTurn;
			return Result;
		}

		if (bMaximizing)
		{
			int32 MaxEval = TNumericLimits<int32>::Min();
			for (int32 Sq : Moves)
			{
				TArray<ECellState> Saved = Board;
				bool bSavedTurn = bIsBlackTurn;
				PlacePiece(Sq / 8, Sq % 8);
				bIsBlackTurn = !bIsBlackTurn;
				int32 Eval = Minimax(Depth - 1, Alpha, Beta, false);
				Board = Saved; bIsBlackTurn = bSavedTurn;
				if (bTimedOut) return 0;
				MaxEval = FMath::Max(MaxEval, Eval);
				Alpha   = FMath::Max(Alpha,   Eval);
				if (Beta <= Alpha) break;
			}
			return MaxEval;
		}
		else
		{
			int32 MinEval = TNumericLimits<int32>::Max();
			for (int32 Sq : Moves)
			{
				TArray<ECellState> Saved = Board;
				bool bSavedTurn = bIsBlackTurn;
				PlacePiece(Sq / 8, Sq % 8);
				bIsBlackTurn = !bIsBlackTurn;
				int32 Eval = Minimax(Depth - 1, Alpha, Beta, true);
				Board = Saved; bIsBlackTurn = bSavedTurn;
				if (bTimedOut) return 0;
				MinEval = FMath::Min(MinEval, Eval);
				Beta    = FMath::Min(Beta,    Eval);
				if (Beta <= Alpha) break;
			}
			return MinEval;
		}
	}

	FString FindBestMove(bool bMaximizing)
	{
		Deadline = FPlatformTime::Seconds() + 2.0;
		const double SearchStart = FPlatformTime::Seconds();

		TArray<int32> ValidMoves = GetValidMoves();
		if (ValidMoves.Num() == 0) return TEXT("");

		int32 BestSquare = ValidMoves[0];
		int32 BestEval   = bMaximizing ? TNumericLimits<int32>::Min() : TNumericLimits<int32>::Max();

		UE_LOG(LogTemp, Log, TEXT("ID Minimax: %s evaluating %d candidates with 2s budget"),
			bMaximizing ? TEXT("Black (max)") : TEXT("White (min)"), ValidMoves.Num());

		for (int32 Depth = 1; Depth <= 20; ++Depth)
		{
			bTimedOut = false;
			const double DepthStart = FPlatformTime::Seconds();
			UE_LOG(LogTemp, Log, TEXT("ID Minimax: starting depth %d (elapsed %.3fs)"),
				Depth, DepthStart - SearchStart);

			int32 DepthBest     = ValidMoves[0];
			int32 DepthBestEval = bMaximizing ? TNumericLimits<int32>::Min() : TNumericLimits<int32>::Max();

			for (int32 Square : ValidMoves)
			{
				if (bTimedOut) break;
				TArray<ECellState> Saved = Board;
				bool bSavedTurn = bIsBlackTurn;
				PlacePiece(Square / 8, Square % 8);
				bIsBlackTurn = !bIsBlackTurn;
				int32 Eval = Minimax(Depth - 1,
					TNumericLimits<int32>::Min(), TNumericLimits<int32>::Max(), !bMaximizing);
				Board = Saved; bIsBlackTurn = bSavedTurn;

				if (!bTimedOut && (bMaximizing ? Eval > DepthBestEval : Eval < DepthBestEval))
				{
					DepthBestEval = Eval;
					DepthBest     = Square;
				}
			}

			if (bTimedOut)
			{
				UE_LOG(LogTemp, Log, TEXT("ID Minimax: timed out at depth %d after %.3fs (total %.3fs)"),
					Depth, FPlatformTime::Seconds() - DepthStart, FPlatformTime::Seconds() - SearchStart);
				break;
			}

			BestSquare = DepthBest;
			BestEval   = DepthBestEval;
			UE_LOG(LogTemp, Log,
				TEXT("ID Minimax: depth %d complete in %.3fs (total %.3fs) — best square %02d score %d"),
				Depth, FPlatformTime::Seconds() - DepthStart, FPlatformTime::Seconds() - SearchStart,
				BestSquare + 1, BestEval);

			if (FPlatformTime::Seconds() >= Deadline) break;
		}

		UE_LOG(LogTemp, Log, TEXT("ID Minimax: chose square %02d with score %d (total %.3fs)"),
			BestSquare + 1, BestEval, FPlatformTime::Seconds() - SearchStart);

		return FString::Printf(TEXT("%02d"), BestSquare + 1);
	}
};
} // namespace

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
	CurrentAIMode = EAIMode::Random;
	bBlackIsAI = false;
	bWhiteIsAI = false;

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


void AGameBoard::SetAIMode(EAIMode Mode)
{
	CurrentAIMode = Mode;
	UE_LOG(LogTemp, Log, TEXT("AI mode set to: %s"), Mode == EAIMode::Minimax ? TEXT("Minimax") : TEXT("Random"));
}

EAIMode AGameBoard::GetAIMode() const
{
	return CurrentAIMode;
}

bool AGameBoard::GetRandomAIMove(FString& OutMove)
{
	TArray<int32> ValidMoves = GetValidMoves();
	if (ValidMoves.Num() == 0) return false;

	if (CurrentAIMode == EAIMode::Minimax)
	{
		FMinimaxSearch Search(Board, bIsBlackTurn);
		OutMove = Search.FindBestMove(bIsBlackTurn);
		return OutMove.Len() > 0;
	}

	const int32 Square = ValidMoves[FMath::RandRange(0, ValidMoves.Num() - 1)];
	OutMove = FString::Printf(TEXT("%02d"), Square + 1);
	return true;
}

EWinResult AGameBoard::GetWinner() const
{
	if (!IsGameOver())
	{
		return EWinResult::InProgress;
	}

	const int32 BlackScore = GetScore(true);
	const int32 WhiteScore = GetScore(false);

	if (BlackScore > WhiteScore)
	{
		UE_LOG(LogTemp, Log, TEXT("Game over: Black wins %d-%d"), BlackScore, WhiteScore);
		return EWinResult::BlackWins;
	}
	if (WhiteScore > BlackScore)
	{
		UE_LOG(LogTemp, Log, TEXT("Game over: White wins %d-%d"), WhiteScore, BlackScore);
		return EWinResult::WhiteWins;
	}

	UE_LOG(LogTemp, Log, TEXT("Game over: Draw %d-%d"), BlackScore, WhiteScore);
	return EWinResult::Draw;
}

bool AGameBoard::PassTurnIfNoMoves()
{
	if (HasAnyValidMove(bIsBlackTurn))
	{
		return false;
	}

	if (IsGameOver())
	{
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("%s has no valid moves — passing turn"),
		bIsBlackTurn ? TEXT("Black") : TEXT("White"));

	bIsBlackTurn = !bIsBlackTurn;
	return true;
}

void AGameBoard::ResetGame()
{
	InitializeBoard();
	UE_LOG(LogTemp, Log, TEXT("Game reset."));
}

int32 AGameBoard::SquareStringToIndex(const FString& Input) const
{
	int32 Index;
	if (!ParseSquareInput(Input, Index))
	{
		return -1;
	}
	return Index + 1;
}

TArray<AActor*> AGameBoard::SortActorsByDisplayName(TArray<AActor*> Actors)
{
	for (const AActor* Actor : Actors)
	{
		if (Actor)
		{
			UE_LOG(LogTemp, Log, TEXT("SortActorsByDisplayName: %s"), *Actor->GetName());
		}
	}

	Actors.Sort([](const AActor& A, const AActor& B)
	{
		return A.GetName() < B.GetName();
	});
	return Actors;
}

bool AGameBoard::CurrentPlayerHasValidMove() const
{
	return HasAnyValidMove(bIsBlackTurn);
}

ETurnState AGameBoard::GetTurnState() const
{
	if (IsGameOver()) return ETurnState::GameOver;
	const bool bCurrentIsAI = bIsBlackTurn ? bBlackIsAI : bWhiteIsAI;
	return bCurrentIsAI ? ETurnState::AITurn : ETurnState::HumanTurn;
}

void AGameBoard::SetBlackIsAI(bool bIsAI)
{
	bBlackIsAI = bIsAI;
	UE_LOG(LogTemp, Log, TEXT("Black is %s"), bIsAI ? TEXT("AI") : TEXT("Human"));
}

void AGameBoard::SetWhiteIsAI(bool bIsAI)
{
	bWhiteIsAI = bIsAI;
	UE_LOG(LogTemp, Log, TEXT("White is %s"), bIsAI ? TEXT("AI") : TEXT("Human"));
}

bool AGameBoard::CurrentPlayer()
{
	return bIsBlackTurn;
}

void AGameBoard::ApplyMove(const FString& Input)
{
	int32 Square;
	if (!ParseSquareInput(Input, Square))
	{
		return;
	}

	if (!PlacePiece(Square / BoardSize, Square % BoardSize))
	{
		return;
	}

	bIsBlackTurn = !bIsBlackTurn;

	PassTurnIfNoMoves();
}
