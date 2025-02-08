struct CrosshairConfig
{
    float thickness;
    int gap;
    int length;
    int color[3];
    float expansion_factor;
    float gap_history[5]; // Store last 5 gap values
    int history_index;
    float current_gap;
    int center_dot;
    int t_style;
    int rgb;
    float rgb_step;
};

struct CrosshairConfig config = {
        .history_index = 0, // unimportant
        .current_gap = 5.0f, // unimportant, controls the current gap
        
        .thickness = 2, // Thickness of the crosshair lines
        .gap = 5, // Gap between the crosshair lines
        .length = 8, // Line length
        .color = {0, 255, 0}, // RGB Color
        .expansion_factor = 0.01f, // Controls how much the gap changes on movement
        .center_dot = 0, // Boolean value for enabling the center dot
        .t_style = 0, // Boolean value for enabling the T-Style
        .rgb = 0, // RGB Boolean (rotating hue)
        .rgb_step = 5.0f, // RGB Speed
};
