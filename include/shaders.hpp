#pragma once

namespace Shaders {

static const char* vertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec2 gridPos;
    uniform vec2 gridCount;
    out vec2 TexCoord;
    flat out int InstanceID;
    void main()
    {
       TexCoord = aPos;
       InstanceID = gl_InstanceID;
       vec2 scale = vec2(2.0 / gridCount.x, 2.0 / gridCount.y);
       vec2 offset = (gridPos / gridCount) * 2.0 - 1.0;
       vec2 finalPos = aPos * scale + offset;
       gl_Position = vec4(finalPos * 0.99, 0.0, 1.0);
    }
)glsl";

static const char* fragmentShaderSource = R"glsl(
    #version 330 core
    out vec4 FragColor;
    in vec2 TexCoord;
    flat in int InstanceID;
    uniform vec3 objectColor;
    uniform bool isApple;
    uniform bool isBanana;
    uniform bool isGoldenApple;
    uniform bool isSnake;
    uniform int snakeDirection;
    uniform float timeLeft;

    void main()
    {
       vec2 p = TexCoord * 2.0 - 1.0;
       if (timeLeft < 1.0 && timeLeft > 0.0) {
           if (TexCoord.y < 0.1 && TexCoord.x > 0.1 && TexCoord.x < 0.9) {
               float barX = (TexCoord.x - 0.1) / 0.8;
               FragColor = (barX < timeLeft) ? vec4(0.0, 1.0, 0.0, 1.0) : vec4(0.3, 0.3, 0.3, 1.0);
               return;
           }
       }
       if (isApple || isGoldenApple) {
           float appleDist = length(vec2(p.x, p.y + 0.1 - 0.2 * abs(p.x)));
           if (appleDist < 0.7) {
               vec3 baseColor = isApple ? vec3(1.0, 0.0, 0.0) : vec3(1.0, 0.85, 0.0);
               vec2 lightPos = vec2(-0.3, 0.3);
               float highlight = pow(max(1.0 - distance(p, lightPos), 0.0), isApple ? 8.0 : 12.0);
               FragColor = vec4(baseColor + vec3(1.0) * highlight * 0.8, 1.0);
           } else if (p.y > 0.4 && p.y < 0.8 && abs(p.x) < 0.05) {
               FragColor = isApple ? vec4(0.0, 0.8, 0.0, 1.0) : vec4(0.4, 0.2, 0.0, 1.0);
           } else discard;
       } else if (isBanana) {
            vec2 q = p; q.x += 0.45 * (q.y * q.y - 0.7);
            float bananaDist = length(vec2(q.x * 2.8, q.y * 0.9)) - 0.75;
            if (bananaDist < 0.0) {
                vec3 bananaColor = mix(vec3(1.0, 0.95, 0.2), vec3(0.9, 0.8, 0.1), 1.0 - max(dot(normalize(vec3(p, 0.5)), vec3(0,0,1)), 0.0));
                float highlight = pow(max(1.0 - distance(p, vec2(-0.3, 0.4)), 0.0), 10.0);
                FragColor = vec4(bananaColor + vec3(0.5) * highlight, 1.0);
            } else if (abs(p.y) > 0.6 && abs(q.x * 2.8) < 0.25) FragColor = vec4(0.2, 0.12, 0.05, 1.0);
            else discard;
       } else if (isSnake) {
           float dist = length(p);
           if (dist > 1.0) discard;
           else {
               float highlight = pow(max(1.0 - distance(p, vec2(-0.5, 0.5)), 0.0), 2.0);
               vec3 finalColor = objectColor * (0.4 + (1.0 - dist) * 0.6) + vec3(1.0) * highlight * 0.3;
               if (InstanceID == 0) {
                   vec2 rotP = p;
                   if (snakeDirection == 1) rotP = -p; 
                   else if (snakeDirection == 2) rotP = vec2(p.y, -p.x);
                   else if (snakeDirection == 3) rotP = vec2(-p.y, p.x);
                   if (length(rotP - vec2(-0.35, 0.4)) < 0.25 || length(rotP - vec2(0.35, 0.4)) < 0.25) {
                       finalColor = vec3(1.0);
                       if (length(rotP - vec2(-0.35, 0.5)) < 0.1 || length(rotP - vec2(0.35, 0.5)) < 0.1) finalColor = vec3(0.0);
                   }
               }
               FragColor = vec4(finalColor, 1.0);
           }
       } else {
           float bevel = smoothstep(0.0, 0.15, min(1.0 - abs(p.x), 1.0 - abs(p.y)));
           FragColor = vec4(objectColor * (0.5 + 0.5 * bevel), 1.0);
       }
    }
)glsl";

static const char* borderVertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    uniform mat4 transform;
    void main() { gl_Position = transform * vec4(aPos, 0.0, 1.0); }
)glsl";

static const char* borderFragmentShaderSource = R"glsl(
    #version 330 core
    out vec4 FragColor;
    uniform vec3 borderColor;
    void main() { FragColor = vec4(borderColor, 1.0); }
)glsl";

static const char* textVertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec4 vertex;
    out vec2 TexCoords;
    uniform mat4 projection;
    void main() { gl_Position = projection * vec4(vertex.xy, 0.0, 1.0); TexCoords = vertex.zw; }
)glsl";

static const char* textFragmentShaderSource = R"glsl(
    #version 330 core
    in vec2 TexCoords;
    out vec4 color;
    uniform sampler2D text;
    uniform vec3 textColor;
    void main() { color = vec4(textColor, 1.0) * vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r); }
)glsl";

} // namespace Shaders