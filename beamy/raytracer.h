#ifndef RAYTRACER_H
#define RAYTRACER_H

#include "tmath.hpp"

#include <vector>
#include <memory>

enum ObjectType {
	TObject = 0,
	TLight
};

class Scene;
struct Ray {
	Vector3 origin, direction;

	Ray() = default;
	Ray(const Vector3& o, const Vector3& d)
		: origin(o), direction(d) {}

	Ray(int x, int y, Scene& scene);

	Vector3 at(float t) const;
};

struct Object {
	Vector3 position;
	Vector3 scale;
	Vector3 color;
	Quaternion rotation;

	Object()
		:	position(Vector3(0, 0, 0)),
			scale(Vector3(1.0f)),
			color(Vector3(1.0f)),
			rotation(Quaternion())
	{}
	~Object() = default;

	Matrix4 getTransformation();

	virtual bool intersects(const Ray& ray, float& t) {
		return false;
	};

	virtual Vector3 normal(const Ray& ray, float t) {
		return Vector3(0.0f);
	}

	virtual ObjectType type() const { return ObjectType::TObject; }
};

struct Sphere : public Object {
	float radius;

	Sphere() = default;

	bool intersects(const Ray& ray, float& t) override;
	Vector3 normal(const Ray& ray, float t) override;
};

struct Plane : public Object {
	Plane() = default;
	
	bool intersects(const Ray& ray, float& t) override;
	Vector3 normal(const Ray& ray, float t) override;

	Vector3 norm;
};

struct Light : public Object {
	float intensity;
	Light() : Object() {}

	ObjectType type() const override { return ObjectType::TLight; }
};

struct Camera : public Object {
	float fov;

	Camera() : Object(), fov(utils::radians(60.0f)) {}
	Matrix4 getView();
};

class Scene {
public:
	Scene() = default;
	~Scene() = default;

	bool intersects(const Ray& ray, float& t, Object** hitObj);

	void add(Object* object);

	Camera& camera() { return m_camera;  }

	int width() const { return m_width; }
	int height() const { return m_height; }

	void width(int w) { m_width = w; }
	void height(int h) { m_height = h; }

	const std::vector<std::unique_ptr<Object>>& objects() const { return m_objects; }

	Vector3 ambient{ 0.0f, 0.0f, 0.0f };

private:
	std::vector<std::unique_ptr<Object>> m_objects;
	Camera m_camera;
	int m_width, m_height;
};

#endif // RAYTRACER_H