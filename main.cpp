#include "tgaimage.h"
#include "model.h"
#include "renderer.h"
#include "transforms.h"
#include <vector>

int main(int argc, char **argv)
{
    TGAImage image(width, height, TGAImage::RGB);
    Renderer renderer(width, height);

    auto afk_face = std::make_shared<Model>("./obj/african_head/african_head.obj");

    auto dirLight = std::make_shared<DirectionalLight>(vec3(0, 1, 1));
    Camera camera = {eye, center, up};
    renderer.setCamera(camera);
    // renderer.model = get_rotation({0, 1, 0}, 180.0);
    renderer.model = mat4::identity();
    renderer.lookat = get_lookAt(eye, center, up);
    renderer.project = get_perspective(float(width) / height, 45, 0.01, 10);
    renderer.viewport = get_viewport(width, height, 1.0);
    renderer.updateMVP();

    Scene scene;
    scene.addLight(dirLight);
    // scene.addLight({0, 0, -1});
    scene.addMesh(std::make_shared<Mesh>(afk_face));

    // render faces
    renderer.render(scene);
    renderer.write_tga_file("face_width_mvp.tga");

    return 0;
}