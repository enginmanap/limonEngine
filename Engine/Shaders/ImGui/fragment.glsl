#version 330 core
#extension GL_ARB_texture_cube_map_array : enable

uniform sampler2D Texture;
uniform sampler2DArray TextureArray;
uniform samplerCubeArray TextureCubeArray;
uniform int isArray;
uniform float layer;


in vec2 Frag_UV;
in vec4 Frag_Color;
out vec4 Out_Color;

void main() {
    if(isArray == 2) {
        if(mod(layer, 6) == 0) {
            Out_Color = Frag_Color * texture( TextureCubeArray, vec4(1.0, Frag_UV.st, (layer)/6.0));
        } else if(mod(layer, 6) == 1) {
            Out_Color = Frag_Color * texture( TextureCubeArray, vec4(Frag_UV.s, 1.0, Frag_UV.t, (layer - 1.0)/6.0));
        } else if(mod(layer, 6) == 2) {
            Out_Color = Frag_Color * texture( TextureCubeArray, vec4(Frag_UV.st, 1.0, (layer - 2.0)/6.0));
        } else if(mod(layer, 6) == 3) {
            Out_Color = Frag_Color * texture( TextureCubeArray, vec4(-1.0, Frag_UV.st, (layer - 3.0)/6.0));
        } else if(mod(layer, 6) == 4) {
            Out_Color = Frag_Color * texture( TextureCubeArray, vec4(Frag_UV.s, -1.0, Frag_UV.t, (layer - 4.0)/6.0));
        } else if(mod(layer, 6) == 5) {
            Out_Color = Frag_Color * texture( TextureCubeArray, vec4(Frag_UV.st, -1.0, (layer - 5.0)/6.0));
        } else {
            Out_Color = vec4(0.0, 0.7, 0.7, 0.0);
        }
    } else if(isArray == 1) {
        Out_Color = Frag_Color * texture(TextureArray, vec3(Frag_UV.st, layer));
    } else {
        Out_Color = Frag_Color * texture(Texture, Frag_UV.st);
    }
}