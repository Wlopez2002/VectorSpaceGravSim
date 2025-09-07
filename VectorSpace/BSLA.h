/*
Bill's Shity Linear Algebra
*/

#pragma once

#include <string>
#include <math.h>

/*
* Class for a two-dimensional vector with some basic linear algebra operations built in.
*/
class Vector2D {
public:
	double x; double y;
	Vector2D() {
		x = 0; y = 0;
	}
	Vector2D(double x1, double y1) {
		x = x1; y = y1;
	}
	Vector2D operator+(Vector2D vect) {
		Vector2D sum;
		sum.x = x + vect.x;
		sum.y = y + vect.y;
		return sum;
	}
	Vector2D operator-(Vector2D vect) {
		Vector2D dif;
		dif.x = x - vect.x;
		dif.y = y - vect.y;
		return dif;
	}
	Vector2D operator*(double scalar) {
		Vector2D product;
		product.x = x * scalar;
		product.y = y * scalar;
		return product;
	}
	bool operator==(Vector2D vect) {
		return (x == vect.x) and (y == vect.y);
	}
	double dot(Vector2D vect) {
		return (x * vect.x) + (y * vect.y);
	}
	double magnitude() {
		return sqrt(abs(x * x) + abs(y * y));
	}
	std::string toString() {
		return "[" + std::to_string(x) + " " + std::to_string(y) + "]";
	}
};

/*
* Class for a 2x2 matrix and some basic operators related to it.
* This class uses the Vector2D class for it's rows.
* 
* |v1.x v1.y|
* |v2.x v2.y| 
*/
class Matrix2D {
public:
	Vector2D v1; Vector2D v2;
	Matrix2D() {
		v1.x = 0; v1.y = 0;
		v2.x = 0; v2.y = 0;
	}
	Matrix2D(double x1, double y1, double x2, double y2) {
		v1.x = x1; v1.y = y1;
		v2.x = x2; v2.y = y2;
	}
	Matrix2D operator+(Matrix2D matrix) {
		Matrix2D sum;
		sum.v1.x = v1.x + matrix.v1.x;
		sum.v1.y = v1.y + matrix.v1.y;
		sum.v2.x = v2.x + matrix.v2.x;
		sum.v2.y = v2.y + matrix.v2.y;
		return sum;
	}
	Matrix2D operator-(Matrix2D matrix) {
		Matrix2D dif;
		dif.v1.x = v1.x - matrix.v1.x;
		dif.v1.y = v1.y - matrix.v1.y;
		dif.v2.x = v2.x - matrix.v2.x;
		dif.v2.y = v2.y - matrix.v2.y;
		return dif;
	}
	Matrix2D operator*(Matrix2D matrix) {
		Matrix2D product;
		product.v1.x = (v1.x * matrix.v1.x) + (v2.x * matrix.v1.y);
		product.v1.y = (v1.y * matrix.v1.x) + (v2.y * matrix.v1.y);
		product.v2.x = (v1.x * matrix.v2.x) + (v2.x * matrix.v2.y);
		product.v2.y = (v1.y * matrix.v2.x) + (v2.y * matrix.v2.y);
		return product;
	}
	Vector2D operator*(Vector2D vector) {
		Vector2D product;
		product.x = (v1.x * vector.x) + (v2.x * vector.y);
		product.y = (v1.y * vector.x) + (v2.y * vector.y);
		return product;
	}
	Matrix2D operator*(double scalar) {
		Matrix2D product;
		product.v1 = v1 * scalar;
		product.v2 = v2 * scalar;
		return product;
	}
	Matrix2D inverse() {
		Matrix2D inverse;
		inverse.v1.x = v2.y;
		inverse.v1.y = -1 * v1.y;
		inverse.v2.x = -1 * v2.x;
		inverse.v2.y = v1.x;
		inverse = inverse * (1.0 / ((v1.x * v2.y) - (v2.x * v1.y)));
		return inverse;
	}
	Matrix2D transpose() {
		Matrix2D transpose;
		transpose.v1.x = v1.x;
		transpose.v1.y = v2.x;
		transpose.v2.x = v1.y;
		transpose.v2.y = v2.y;
		return transpose;
	}
	double det() {
		return (v1.x * v2.y) - (v2.x * v1.y);
	}
	std::string toString() {
		return "[" + std::to_string(v1.x) + " " + std::to_string(v1.y) + "]" + "[" + std::to_string(v2.x) + " " + std::to_string(v2.y) + "]";
	}
};