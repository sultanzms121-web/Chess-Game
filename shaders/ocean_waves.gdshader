shader_type canvas_item;

uniform float time_scale = 0.6;
uniform vec4 wave_color : source_color = vec4(0.9, 0.95, 1.0, 1.0); // Metallic white/silver
uniform vec4 bg_color : source_color = vec4(0.01, 0.01, 0.01, 1.0); // Deep black

void fragment() {
    vec2 uv = UV;
    float t = TIME * time_scale;
    
    // Procedural waves using sine combinations
    float wave1 = sin(uv.x * 6.0 + t) * 0.1;
    float wave2 = cos(uv.x * 12.0 - t * 1.5) * 0.05;
    float wave3 = sin(uv.x * 4.0 + t * 0.5 + wave2 * 2.0) * 0.15;
    
    float combined_wave = wave1 + wave2 + wave3;
    
    // Distance to the wave line
    float dist = abs(uv.y - 0.5 - combined_wave);
    
    // Create a glowing, metallic sharp line
    float glow = 0.01 / (dist + 0.005);
    glow = clamp(glow, 0.0, 1.0);
    
    // Add some noise or shimmer along the wave
    float shimmer = sin(uv.x * 30.0 + t * 4.0) * 0.5 + 0.5;
    glow *= mix(0.7, 1.5, shimmer);
    
    // Add multiple background waves faintly
    float bg_wave1 = sin(uv.x * 8.0 - t * 0.8) * 0.2;
    float bg_dist1 = abs(uv.y - 0.3 - bg_wave1);
    float bg_glow1 = 0.005 / (bg_dist1 + 0.01);
    
    float bg_wave2 = cos(uv.x * 5.0 + t * 1.2) * 0.25;
    float bg_dist2 = abs(uv.y - 0.8 - bg_wave2);
    float bg_glow2 = 0.005 / (bg_dist2 + 0.01);
    
    float total_glow = clamp(glow + bg_glow1 * 0.4 + bg_glow2 * 0.4, 0.0, 1.0);
    
    // Blend with background
    COLOR = mix(bg_color, wave_color, total_glow);
}
