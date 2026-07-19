shader_type canvas_item;

uniform float time_scale = 1.5;
uniform vec4 wave_color : source_color = vec4(1.0, 1.0, 1.0, 1.0); // Pure White
uniform vec4 base_color : source_color = vec4(0.9, 0.9, 0.9, 1.0); // Bright Gray

void fragment() {
    vec4 tex_color = texture(TEXTURE, UV);
    
    float t = TIME * time_scale;
    
    // Diagonal wave sweep, smoother
    float wave = sin(UV.x * 8.0 + UV.y * 8.0 - t * 2.5);
    float band = smoothstep(0.4, 1.0, wave); 
    
    // High frequency Shimmer, softer
    float shimmer = sin(UV.x * 30.0 - UV.y * 20.0 + t * 6.0) * 0.5 + 0.5;
    
    // Final glow multiplier, breathing effect
    float breath = sin(t * 1.5) * 0.2 + 0.8;
    float glow = (1.5 + (band * 2.0) + (shimmer * 0.5)) * breath; // BOOSTED GLOW
    
    // Mix the base color with the bright wave color
    vec3 final_rgb = mix(base_color.rgb, wave_color.rgb, band + shimmer * 0.5);
    final_rgb *= glow;
    
    // Apply texture alpha, completely ignoring original darkness
    COLOR = vec4(final_rgb * COLOR.rgb, tex_color.a * COLOR.a);
}

