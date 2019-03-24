#include "raytracer.h"

#include <unordered_map>
#include <algorithm>

bool Sphere::intersects(const Ray& ray, float& t) {
	Vector3 oc = ray.origin - position;
	float a = ray.direction.dot(ray.direction);
	float b = 2.0f * oc.dot(ray.direction);
	float c = oc.dot(oc) - radius * radius;
	float dis = b * b - 4.0f * a * c;
	if (dis < 0.0f) {
		t = -1.0f;
		return false;
	}
	t = (-b - ::sqrtf(dis)) / (2.0f * a);
	return true;
}

Vector3 Sphere::normal(const Ray& ray, float t) {
	return (ray.at(t) - position).normalized();
}

bool Scene::intersects(const Ray& ray, float& t, Object** hitObj) {
	using DistPair = std::pair<size_t, float>;
	std::vector<DistPair> distances;

	size_t i = 0;
	float st = 0.0f;
	for (auto&& ob : m_objects) {
		if (ob->type() != TLight && ob->intersects(ray, st)) {
			distances.push_back(std::make_pair(i, st));
		}
		i++;
	}

	std::sort(distances.begin(), distances.end(), [](const DistPair& a, const DistPair& b) {
		return a.second < b.second;
	});

	if (!distances.empty()) {
		if (hitObj) *hitObj = m_objects[distances[0].first].get();
		t = distances[0].second;
		return true;
	}

	if (hitObj) *hitObj = nullptr;
	t = -1.0f;
	return false;
}

void Scene::add(Object* object) {
	m_objects.push_back(std::unique_ptr<Object>(object));
}

Matrix4 Object::getTransformation() {
	Matrix4 T = Matrix4::translation(position);
	Matrix4 R = rotation.toMatrix4();
	Matrix4 S = Matrix4::scale(scale);
	return T * R * S;
}

Matrix4 Camera::getView() {
	Matrix4 T = Matrix4::translation(position * -1.0f);
	Matrix4 R = rotation.conjugated().toMatrix4();
	return T * R;
}

Ray::Ray(int x, int y, Scene& scene) {
	float w = float(scene.width());
	float h = float(scene.height());
	float fova = ::tanf(scene.camera().fov / 2.0f);
	float aspect = w / h;
	float sx = ((((float(x) + 0.5f) / w) * 2.0f - 1.0f) * aspect) * fova;
	float sy = (1.0f - ((float(y) + 0.5f) / h) * 2.0f) * fova;

	direction = (scene.camera().getView() * Vector4(sx, sy, -1.0f, 0.0f).normalized()).toVector3();
	origin = scene.camera().position * -1.0f;
}

Vector3 Ray::at(float t) const {
	return origin + direction * t;
}

bool Plane::intersects(const Ray& ray, float& t) {
	float denom = norm.dot(ray.direction);
	if (denom > consts::Epsilon) {
		Vector3 p0l0 = position - ray.origin;
		t = p0l0.dot(norm) / denom;
		if (t >= 0.0f) {
			return true;
		}
	}
	t = -1.0f;
	return false;
}

Vector3 Plane::normal(const Ray& ray, float t) {
	return norm.normalized() * -1;
}
