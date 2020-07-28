/* ---------------------------------------------------------------------------

 Polynomial regression, freely adapted from:
 
 NUMERICAL METHODS: C Programs, (c) John H. Mathews 1995
 Algorithm translated to  C  by: Dr. Norman Fahrer
 NUMERICAL METHODS for Mathematics, Science and Engineering, 2nd Ed, 1992
 Prentice Hall, International Editions:   ISBN 0-13-625047-5
 This free software is compliments of the author.
 E-mail address:       in%"mathews@fullerton.edu"
*/

#include <math.h>

#define DMAX 5	/* Maximum degree of polynomial */
#define NMAX 10	/* Maximum number of points */

static void FactPiv(int N, double A[DMAX][DMAX], double B[], double Cf[]);

void polyreg(const int M, const int N, const double X[], const double Y[], double C[]) {
	int R, K, J;			/* Loop counters */
	double A[DMAX][DMAX];	/* A */
	double B[DMAX];
	double P[2 * DMAX + 1];
	double x, y;
	double p;

	/* Zero the array */
	for (R = 0; R < M + 1; R++)
		B[R] = 0;

	/* Compute the column vector */
	for (K = 0; K < N; K++) {
		y = Y[K];
		x = X[K];
		p = 1.0;
		for (R = 0; R < M + 1; R++) {
			B[R] += y * p;
			p = p * x;
		}
	}

	/* Zero the array */
	for (J = 1; J <= 2 * M; J++)
		P[J] = 0;

	P[0] = N;

	/* Compute the sum of powers of x_(K-1) */
	for (K = 0; K < N; K++) {
		x = X[K];
		p = X[K];
		for (J = 1; J <= 2 * M; J++) {
			P[J] += p;
			p = p * x;
		}
	}

	/* Determine the matrix entries */
	for (R = 0; R < M + 1; R++) {
		for (K = 0; K < M + 1; K++)
			A[R][K] = P[R + K];
	}

	/* Solve the linear system of M + 1 equations: A*C = B 
	   for the coefficient vector C = (c_1,c_2,..,c_M,c_(M+1)) */
	FactPiv(M + 1, A, B, C);
}	/* end main */


/*--------------------------------------------------------*/
static void FactPiv(int N, double A[DMAX][DMAX], double B[], double Cf[]) {
	int K, P, C, J;		/* Loop counters		 */
	int Row[NMAX];		/* Field with row-number */
	double X[DMAX], Y[DMAX];
	double SUM, DET = 1.0;
	int T;

	/* Initialize the pointer vector */
	for (J = 0; J < NMAX; J++)
		Row[J] = J;

	/* Start LU factorization */
	for (P = 0; P < N - 1; P++) {
		/* Find pivot element */
		for (K = P + 1; K < N; K++) {
			if (fabs(A[Row[K]][P]) > fabs(A[Row[P]][P])) {
				/* Switch the index for the p-1 th pivot row if necessary */
				T = Row[P];
				Row[P] = Row[K];
				Row[K] = T;
				DET = -DET;
			}
		}	/* End of simulated row interchange */
		if (A[Row[P]][P] == 0) {
			/* The matrix is SINGULAR! */
			return;
		}

		/* Multiply the diagonal elements */
		DET = DET * A[Row[P]][P];

		/* Form multiplier */
		for (K = P + 1; K < N; K++) {
			A[Row[K]][P] = A[Row[K]][P] / A[Row[P]][P];

			/* Eliminate X_(p-1) */
			for (C = P + 1; C < N + 1; C++) {
				A[Row[K]][C] -= A[Row[K]][P] * A[Row[P]][C];
			}
		}
	}
	/* End of  L*U  factorization routine */
	DET = DET * A[Row[N - 1]][N - 1];

	/* Start the forward substitution */
	for (K = 0; K < N; K++)
		Y[K] = B[K];

	Y[0] = B[Row[0]];

	for (K = 1; K < N; K++) {
		SUM = 0;
		for (C = 0; C <= K - 1; C++)
			SUM += A[Row[K]][C] * Y[C];
		Y[K] = B[Row[K]] - SUM;
	}

	/* Start the back substitution */
	X[N - 1] = Y[N - 1] / A[Row[N - 1]][N - 1];
	for (K = N - 2; K >= 0; K--) {
		SUM = 0;
		for (C = K + 1; C < N; C++) {
			SUM += A[Row[K]][C] * X[C];
		}
		X[K] = (Y[K] - SUM) / A[Row[K]][K];
	}	/* End of back substitution */

	/* Output */
	for (K = 0; K < N; K++)
		Cf[K] = X[K];
}
