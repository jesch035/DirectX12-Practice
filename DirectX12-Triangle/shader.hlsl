struct PSInput
{
    float4 position : SV_Position;
    float3 color : COLOR;
};

cbuffer cb0 : register(b0)
{
    uint angle;
}

// vertex shader
PSInput VSMain(uint index : SV_VertexID)
{
    PSInput output;
    
    static float2 positions[3] =
    {
        float2(-0.43f, -0.25f),
        float2(0.0f, 0.5f),
        float2(0.43f, -0.25f)
    };
    
    static float3 colors[3] =
    {
        float3(1.0f, 0.0f, 0.0f),
        float3(0.0f, 1.0f, 0.0f),
        float3(0.0f, 0.0f, 1.0f)
    };
    
    float2 inputPos = positions[index];
    
    // compute the rotation matrix
    float rotationSpeed = -0.01f;
    
    float cosTheta = cos(angle * rotationSpeed);
    float sinTheta = sin(angle * rotationSpeed);
    
    float2 rotatedPos;
    rotatedPos.x = inputPos.x * cosTheta - inputPos.y * sinTheta;
    rotatedPos.y = inputPos.x * sinTheta + inputPos.y * cosTheta;
    
    output.position = float4(rotatedPos.x, rotatedPos.y, 0.0f, 1.0f);
    output.color = colors[index];
    return output;
}

// pixel shader
float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(input.color.r, input.color.g, input.color.b, 1.0f);
}