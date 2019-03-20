#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <ctime>
#include <omp.h>

#include "json.hpp"
#include "raytracer.h"
#include "stb_image_write.h"

constexpr int Width = 512;
constexpr int Height = 512;

using Json = nlohmann::json;

#define BEGIN_BENCH std::clock()
#define END_BENCH(clk, name) std::cout << name << " time" << ": " << \
    (std::clock() - clk) / CLOCKS_PER_SEC << " secs. (" << (std::clock() - clk) << " ms)" << std::endl; 

struct RenderTile {
	int x, y, width, height;
};

int main() {
	std::ifstream sce("scene.json");
	Json obj;
	sce >> obj;
	sce.close();

	Scene scene{};
	scene.width(obj.value("width", Width));
	scene.height(obj.value("height", Width));

	if (obj["ambient"].is_array()) {
		Json col = obj["ambient"];
		scene.ambient = Vector3(col[0], col[1], col[2]);
	}

	std::vector<unsigned char> pixels;
	pixels.resize(scene.width() * scene.height() * 3);

	if (obj["camera"].is_object()) {
		Json cam = obj["camera"];
		scene.camera().fov = utils::radians(cam.value("fov", 60.0f));
		
		if (cam["position"].is_array()) {
			Json pos = cam["position"];
			scene.camera().position = Vector3(pos[0], pos[1], pos[2]);
		}
		if (cam["rotation"].is_array()) {
			Json rot = cam["rotation"];
			Quaternion x, y, z;
			x = Quaternion::axisAngle(Vector3(1, 0, 0), utils::radians(rot[0]));
			y = Quaternion::axisAngle(Vector3(0, 1, 0), utils::radians(rot[1]));
			z = Quaternion::axisAngle(Vector3(0, 0, 1), utils::radians(rot[2]));
			scene.camera().rotation = x * y * z;
		}
	}

	// Read objects
	if (obj["objects"].is_array()) {
		for (int i = 0; i < obj["objects"].size(); i++) {
			Json ob = obj["objects"][i];
			std::string type = ob.value("type", "sphere");

			Object* object;
			if (type == "sphere") {
				object = new Sphere();
				dynamic_cast<Sphere*>(object)->radius = ob.value("radius", 1.0f);
			} else if (type == "plane") {
				object = new Plane();
			} else if (type == "light") {
				object = new Light();
				dynamic_cast<Light*>(object)->intensity = ob.value("intensity", 1.0f);
			} else {
				continue;
			}

			if (ob["position"].is_array()) {
				Json pos = ob["position"];
				object->position = Vector3(pos[0], pos[1], pos[2]);
			}
			if (ob["rotation"].is_array()) {
				Json rot = ob["rotation"];
				Quaternion x, y, z;
				x = Quaternion::axisAngle(Vector3(1, 0, 0), utils::radians(rot[0]));
				y = Quaternion::axisAngle(Vector3(0, 1, 0), utils::radians(rot[1]));
				z = Quaternion::axisAngle(Vector3(0, 0, 1), utils::radians(rot[2]));
				object->rotation = x * y * z;
			}
			if (ob["scale"].is_array()) {
				Json scl = ob["scale"];
				object->scale = Vector3(scl[0], scl[1], scl[2]);
			}
			if (ob["color"].is_array()) {
				Json col = ob["color"];
				object->color = Vector3(col[0], col[1], col[2]);
			}

			scene.add(object);
		}
	}

	// Generate tiles
	int tileSize = obj.value("tileSize", 32);
	std::vector<RenderTile> tiles;
	for (int y = 0; y < scene.height(); y+=tileSize) {
		for (int x = 0; x < scene.width(); x+=tileSize) {
			RenderTile rt{};
			rt.x = x;
			rt.y = y;
			rt.width = tileSize;
			rt.height = tileSize;
			tiles.push_back(rt);
		}
	}

	std::cout << "Rendering " << tiles.size() << " tiles..." << std::endl;

	auto clk = BEGIN_BENCH;

	// Render
	//#pragma omp parallel for schedule(dynamic)
	for (int k = 0; k < tiles.size(); k++) {
		RenderTile rt = tiles[k];
		for (int y = rt.y; y < rt.y + rt.height; y++) {
			for (int x = rt.x; x < rt.x + rt.width; x++) {
				if (x < 0 || x >= scene.width() ||
					y < 0 || y >= scene.height())
					continue;

				int i = (x + y * scene.width()) * 3;

				Vector3 color(0.0f);
				Ray ray(x, y, scene);

				float t = -1.0f;
				Object* hit = nullptr;
				if (scene.intersects(ray, t, &hit)) {
					Vector3 ip = ray.at(t);
					Vector3 in = hit->normal(ray, t);
					Vector3 lighting = scene.ambient;

					for (auto&& light : scene.objects()) {
						if (light->type() != TLight) continue;

						Light* l = dynamic_cast<Light*>(light.get());
						Vector3 L = (light->position - ip);

						// Cast shadow ray
						//Ray sray(ip + in * consts::Epsilon, L.normalized());
						//if (scene.intersects(sray, t, nullptr)) continue;
						//

						float dist = L.length();
						L = L.normalized();

						float nl = std::clamp(in.dot(L), 0.0f, 1.0f);
						lighting += light->color * (nl * l->intensity / dist * dist);
					}

					color = hit->color * lighting;
				}

				int r = std::clamp(int(color.x * 255.0f), 0, 255);
				int g = std::clamp(int(color.y * 255.0f), 0, 255);
				int b = std::clamp(int(color.z * 255.0f), 0, 255);
				pixels[i + 0] = r;
				pixels[i + 1] = g;
				pixels[i + 2] = b;
			}
		}
	}

	END_BENCH(clk, "Rendering");
	
	stbi_write_png("out.png", scene.width(), scene.height(), 3, pixels.data(), 0);

	std::cin.get();
	return 0;
}