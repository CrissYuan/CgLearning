const double myPi = 3.14159265358979323846;

void buildPerspectiveMatrix(double, double,
	double, double, float m[16]);

void buildLookAtMatrix(double eyex, double eyey, double eyez,
	double centerx, double centery, double centerz,
	double upx, double upy, double upz,
	float m[16]);

void makeRotateMatrix(float angle,
	float ax, float ay, float az,
	float m[16]);

void makeTranslateMatrix(float x, float y, float z, float m[16]);

void multMatrix(float dst[16],
	const float src1[16], const float src2[16]);

void invertMatrix(float *out, const float *m);

void transform(float dst[4], const float mat[16], const float vec[4]);