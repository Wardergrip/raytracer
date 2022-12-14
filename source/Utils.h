#pragma once
#include <cassert>
#include <fstream>
#include "Math.h"
#include "DataTypes.h"

#define MOLLER_TRUMBORE

namespace dae
{
	namespace GeometryUtils
	{
#pragma region Sphere HitTest
		//SPHERE HIT-TESTS
		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			Vector3 vectorDiff = ray.origin - sphere.origin;

			float A{ Vector3::Dot(ray.direction,ray.direction) };
			float B{ Vector3::Dot((2*ray.direction),vectorDiff) };
			float C{ Vector3::Dot(vectorDiff,vectorDiff) - (sphere.radius * sphere.radius)};

			float D{ (B * B) - (4 * A * C) };
			if (D <= 0) // No hit
			{
				hitRecord.didHit = false;
				return false;
			}
			else if (D > 0) // 2 hits
			{
				float t{ ((- B - sqrtf(D)) / (2 * A))};
				if (t < ray.min)
				{
					t = ((- B + sqrtf(D)) / (2 * A));
				}
				if (t > ray.min && t < ray.max)
				{
					hitRecord.didHit = true;
					hitRecord.materialIndex = sphere.materialIndex;
					hitRecord.origin = ray.origin + (t * ray.direction);
					hitRecord.normal = (hitRecord.origin - sphere.origin).Normalized();
					hitRecord.t = t;
					return true;
				}
				hitRecord.didHit = false;
			}
			return false;
		}

		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Sphere(sphere, ray, temp, true);
		}
#pragma endregion
#pragma region Plane HitTest
		//PLANE HIT-TESTS
		inline bool HitTest_Plane(const Plane& plane, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			const float t = Vector3::Dot((plane.origin - ray.origin), plane.normal) / Vector3::Dot(ray.direction,plane.normal);
			if (t > ray.min && t < ray.max)
			{
				hitRecord.origin = (ray.origin + ray.direction * t);
				hitRecord.normal = plane.normal;
				hitRecord.materialIndex = plane.materialIndex;
				hitRecord.t = t;
				hitRecord.didHit = true;
				return true;
			}
			hitRecord.didHit = false;
			return false;
		}

		inline bool HitTest_Plane(const Plane& plane, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Plane(plane, ray, temp, true);
		}
#pragma endregion
#pragma region Triangle HitTest
		inline bool SlabTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray)
		{
			const Vector3 inversedDirection = { 1.f / ray.direction.x,1.f / ray.direction.y,1.f / ray.direction.z };
			const float tx1 = (mesh.transformedMinAABB.x - ray.origin.x) * inversedDirection.x;
			const float tx2 = (mesh.transformedMaxAABB.x - ray.origin.x) * inversedDirection.x;

			float tmin = std::min(tx1, tx2);
			float tmax = std::max(tx1, tx2);

			const float ty1 = (mesh.transformedMinAABB.y - ray.origin.y) * inversedDirection.y;
			const float ty2 = (mesh.transformedMaxAABB.y - ray.origin.y) * inversedDirection.y;

			tmin = std::max(tmin, std::min(ty1, ty2));
			tmax = std::min(tmax, std::max(ty1, ty2));

			const float tz1 = (mesh.transformedMinAABB.z - ray.origin.z) * inversedDirection.z;
			const float tz2 = (mesh.transformedMaxAABB.z - ray.origin.z) * inversedDirection.z;

			tmin = std::max(tmin, std::min(tz1, tz2));
			tmax = std::min(tmax, std::max(tz1, tz2));

			return tmax > 0 && tmax >= tmin;
		}

		//TRIANGLE HIT-TESTS
		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			TriangleCullMode cullMode{ triangle.cullMode };
			if (ignoreHitRecord)
			{
				switch (triangle.cullMode)
				{
				case TriangleCullMode::BackFaceCulling:
					cullMode = TriangleCullMode::FrontFaceCulling;
					break;
				case TriangleCullMode::FrontFaceCulling:
					cullMode = TriangleCullMode::BackFaceCulling;
					break;
				}
			}
			switch (cullMode)
			{
			case TriangleCullMode::BackFaceCulling:
				if (Vector3::Dot(triangle.normal, ray.direction) > 0)
				{
					return false;
				}
				break;
			case TriangleCullMode::FrontFaceCulling:
				if (Vector3::Dot(triangle.normal, ray.direction) < 0)
				{
					return false;
				}
				break;
			case TriangleCullMode::NoCulling:
				break;
			}
#ifdef MOLLER_TRUMBORE
			// Source: https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm

			Vector3 edge1, edge2, h, s, q;
			float a, f, u, v;
			edge1 = triangle.v1 - triangle.v0;
			edge2 = triangle.v2 - triangle.v0;
			h = Vector3::Cross(ray.direction, edge2);
			a = Vector3::Dot(edge1, h);
			if (a > -FLT_EPSILON && a < FLT_EPSILON)
				return false;    // This ray is parallel to this triangle.
			f = 1.0 / a;
			s = ray.origin - triangle.v0;
			u = f * Vector3::Dot(s, h);
			if (u < 0.0 || u > 1.0)
				return false;
			q = Vector3::Cross(s, edge1);
			v = f * Vector3::Dot(ray.direction, q);
			if (v < 0.0 || u + v > 1.0)
				return false;
			// At this stage we can compute t to find out where the intersection point is on the line.
			float t = f * Vector3::Dot(edge2, q);
			if (t > ray.min && t < ray.max) // ray intersection
			{
				hitRecord.origin = ray.origin + ray.direction * t;
				hitRecord.didHit = true;
				hitRecord.materialIndex = triangle.materialIndex;
				hitRecord.normal = triangle.normal;
				hitRecord.t = t;
				return true;
			}
			else // This means that there is a line intersection but not a ray intersection.
				return false;
#else // No Moller Trumbore
			Vector3 center = ((triangle.v0 + triangle.v1 + triangle.v2) / 3);
			Vector3 a{ triangle.v1 - triangle.v0 };
			Vector3 b{ triangle.v2 - triangle.v0 };
			Vector3 normal = Vector3::Cross(a, b);
			if (Vector3::Dot(normal, ray.direction) == 0)
				return false;
			Vector3 L{ center - ray.origin };
			float t = Vector3::Dot(L, normal) / Vector3::Dot(ray.direction, normal);
			if (t < ray.min || t > ray.max)
				return false;
			Vector3 p = ray.origin + t * ray.direction;

			Vector3 c{ p - triangle.v0 };

			Vector3 edgeA{ triangle.v1 - triangle.v0 };
			Vector3 edgeB{ triangle.v2 - triangle.v1 };
			Vector3 edgeC{ triangle.v0 - triangle.v2 };
			Vector3 pointToSide{ p - triangle.v0 };
			if (Vector3::Dot(normal, Vector3::Cross(edgeA, pointToSide)) < 0)
				return false;
			pointToSide = p - triangle.v1;
			if (Vector3::Dot(normal, Vector3::Cross(edgeB, pointToSide)) < 0)
				return false;
			pointToSide = p - triangle.v2;
			if (Vector3::Dot(normal, Vector3::Cross(edgeC, pointToSide)) < 0)
				return false;
			
			//Fill in HitRecord
			hitRecord.didHit = true;
			hitRecord.materialIndex = triangle.materialIndex;
			hitRecord.normal = triangle.normal;
			hitRecord.t = t;
			hitRecord.origin = p;
			return true;
#endif
		}

		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Triangle(triangle, ray, temp, true);
		}
#pragma endregion
#pragma region TriangeMesh HitTest
		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			if (mesh.indices.size() % 3) return false;

			// Slabtest
			if (!SlabTest_TriangleMesh(mesh, ray))
			{
				return false;
			}
			HitRecord smallestTRecord;
			smallestTRecord.t = FLT_MAX;
			HitRecord currentRecord;
			bool hitAtleastOne{ false };
			size_t amountOfTriangles{ mesh.indices.size() / 3 };
			Triangle triangle;
			for (size_t i{ 0 }; i < amountOfTriangles; ++i)
			{
				const size_t index{ (i * 3) };

				triangle = { mesh.transformedPositions[mesh.indices[index]], mesh.transformedPositions[mesh.indices[index + 1]], mesh.transformedPositions[mesh.indices[index + 2]] };
				triangle.cullMode = mesh.cullMode; 
				triangle.materialIndex = mesh.materialIndex;
				triangle.normal = mesh.transformedNormals[i];
				if (HitTest_Triangle(triangle, ray, currentRecord, ignoreHitRecord))
				{
					if (currentRecord.t < smallestTRecord.t)
					{
						smallestTRecord = currentRecord;
					}
					hitAtleastOne = true;
				}
			}
			hitRecord = smallestTRecord;
			return hitAtleastOne;
		}

		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_TriangleMesh(mesh, ray, temp, true);
		}
#pragma endregion
	}

	namespace LightUtils
	{
		//Direction from target to light
		inline Vector3 GetDirectionToLight(const Light& light, const Vector3& origin)
		{
			switch (light.type)
			{
			case LightType::Point:
				return (light.origin - origin);
				break;
			case LightType::Directional:
				return (light.origin - origin);
				break;
			}
		}

		inline ColorRGB GetRadiance(const Light& light, const Vector3& target)
		{
			switch (light.type)
			{
			case LightType::Point:
				return {(light.color * light.intensity) / GetDirectionToLight(light,target).SqrMagnitude()};
				break;
			case LightType::Directional:
				return {light.color * light.intensity};
				break;
			}
		}
	}

	namespace Utils
	{
		//Just parses vertices and indices
#pragma warning(push)
#pragma warning(disable : 4505) //Warning unreferenced local function
		static bool ParseOBJ(const std::string& filename, std::vector<Vector3>& positions, std::vector<Vector3>& normals, std::vector<int>& indices)
		{
			std::ifstream file(filename);
			if (!file)
				return false;

			std::string sCommand;
			// start a while iteration ending when the end of file is reached (ios::eof)
			while (!file.eof())
			{
				//read the first word of the string, use the >> operator (istream::operator>>) 
				file >> sCommand;
				//use conditional statements to process the different commands	
				if (sCommand == "#")
				{
					// Ignore Comment
				}
				else if (sCommand == "v")
				{
					//Vertex
					float x, y, z;
					file >> x >> y >> z;
					positions.push_back({ x, y, z });
				}
				else if (sCommand == "f")
				{
					float i0, i1, i2;
					file >> i0 >> i1 >> i2;

					indices.push_back((int)i0 - 1);
					indices.push_back((int)i1 - 1);
					indices.push_back((int)i2 - 1);
				}
				//read till end of line and ignore all remaining chars
				file.ignore(1000, '\n');

				if (file.eof()) 
					break;
			}

			//Precompute normals
			for (uint64_t index = 0; index < indices.size(); index += 3)
			{
				uint32_t i0 = indices[index];
				uint32_t i1 = indices[index + 1];
				uint32_t i2 = indices[index + 2];

				Vector3 edgeV0V1 = positions[i1] - positions[i0];
				Vector3 edgeV0V2 = positions[i2] - positions[i0];
				Vector3 normal = Vector3::Cross(edgeV0V1, edgeV0V2);

				if(isnan(normal.x))
				{
					int k = 0;
				}

				normal.Normalize();
				if (isnan(normal.x))
				{
					int k = 0;
				}

				normals.push_back(normal);
			}

			return true;
		}
#pragma warning(pop)
	}
}