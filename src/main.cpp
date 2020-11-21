#include <iostream>
#include <chrono>
#include <thread>
#include "Context.h"
#include "InputMonitor.h"
#include "Shader.h"
#include "Texture.h"
#include "Image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

class Canvas : public kki::Texture{
private:
    kki::Image& _img;
    std::vector<float> _grayscale;
    const int _convolution_size;

    float& get(size_t x, size_t y) { return _grayscale[y * width + x]; }

public:

    Canvas(int width, int height, kki::Image& img, int convolution_size)
        :   kki::Texture(width, height), _img(img), _convolution_size(convolution_size)
    {
        Texture& tex = *this;
        _grayscale.reserve(width * height);
        for (int i = 0; i < width * height; ++i){
//            tex(i) = glm::vec3{1.f, 1.f, 1.f} - img(i);
            const auto& colour = img(i);
            float value = colour[0] * 0.2126f + colour[1] * 0.7152f + colour[2] * 0.0722f;
            _grayscale.push_back(value);
            tex(i) = {value, value, value};
        }
        updateTexture();
    }

    void update() override {
        Texture& tex = *this;
        _grayscale.reserve(width * height);

        const int start = -_convolution_size/2;
        const int end = _convolution_size + start;

        for (int y = 0; y < height; ++y){
            for (int x = 0; x < width; ++x){

                // Get sum of all elements
                glm::vec3 sum{};
                size_t total = 0;
                for(int _y = y + start; _y < y + end; ++_y){
                    if (_y < 0 || _y >= height) continue;
                    for(int _x = x + start; _x < x + end; ++_x){
                        if (_x < 0 || _x >= width) continue;
                        sum += at(_x, _y);
                        ++total;
                    }
                }

                glm::vec3 val = _img(x, y);
                glm::vec3 expected_sum = val * static_cast<float>(total);

                glm::vec3 difference = expected_sum - sum;
                glm::vec3 value = glm::min(glm::max(val + difference, 0.f), 1.f);

                at(x, y) = value;
            }
        }
        changed = true;
    }

    void save(const std::string& path = "image.png"){
        size_t data_size = width * height * 3;
        auto* data = new unsigned char[data_size];
        auto* iter = data;
        for (unsigned i = 0; i < width * height; ++i, iter += 3){
            const auto& colour = at(i);
            for(unsigned j = 0; j < 3; ++j){
                iter[j] = static_cast<unsigned char>(colour[j] * 255.f);
            }
        }
        stbi_flip_vertically_on_write(true);
        stbi_write_png(path.c_str(), width, height, 3, data, 3 * width);
        delete [] data;
    }
};

int main()
{
    kki::Image img{"../img/jungle.jpg"};

    int width               = img.getWidth();
    int height              = img.getHeight();

    int background_scale    = 3;

    // Duration of frame
    std::chrono::milliseconds frame_len(30);

    // Window init
    kki::Context window(width, height, "Autopic");
    kki::InputMonitor input ({GLFW_KEY_LEFT_CONTROL, GLFW_KEY_S}, window);
    window.bind();

    Canvas canvas(width, height, img, 7);

    // Shaders init
    auto shader = std::make_shared<kki::Shader>("../src/shaders/pixies.glsl");
    auto background_shader = std::make_shared<kki::Shader>("../src/shaders/texture.glsl");
    background_shader->use();

    bool hold = true;
    while (!window.shouldClose())
    {
        auto begin = std::chrono::steady_clock::now();
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        canvas.update();
        canvas.draw(background_shader);

        window.swapBuffers();
        auto end = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(frame_len - (end - begin));
        window.resetViewport();
        window.pollEvents();

        if ( input[GLFW_KEY_LEFT_CONTROL] && input[GLFW_KEY_S] ){
            if(!hold){
                canvas.save();
                std::cout << "Image saved!" << std::endl;
                hold = true;
            }
        }
        else{
            hold = false;
        }
    }
    return 0;
}
