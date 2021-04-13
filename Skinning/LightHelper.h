#pragma once
#include "CVector3.h"

struct Material {
	//Material() { ZeroMemory(this, sizeof(this)); }
	CVector3 Ambient;
	float pad1;

	CVector3 Diffuse;
	float pad2;

	CVector3 Specular; // w = SpecPower
	float pad3; 

	CVector3 Reflect;
	float pad4;
};

struct DirectionalLight {
	CVector3 Ambient;
	float pad1;

	CVector3 Diffuse;
	float pad2;

	CVector3 Specular;
	float pad3; 
	CVector3 Direction;
	float Pad; // Pad the last float so we can
			   // array of lights if we wanted.
};
struct PointLight{
	CVector3 Ambient;
	float pad1;

	CVector3 Diffuse;
	float pad2;

	CVector3 Specular;
	float pad3;
	
	// Packed into 4D vector: (Position, Range)
	CVector3 Position;
	float Range;
	
	// Packed into 4D vector: (A0, A1, A2, Pad)
	CVector3 Att;
	float Pad; // Pad the last float so we can set an
			   // array of lights if we wanted.
};
struct SpotLight{

	CVector3 Ambient;
	float pad1;

	CVector3 Diffuse;
	float pad2;

	CVector3 Specular;
	float pad3;

	// Packed into 4D vector: (Position, Range)
	CVector3 Position;
	float Range;// Packed into 4D vector: (Direction,Spot)


	CVector3 Direction;
	float Spot;// Packed into 4D vector: (Att, Pad)


	CVector3 Att;
	float Pad; // Pad the last float so we can set an// array of lights if we wanted.
};