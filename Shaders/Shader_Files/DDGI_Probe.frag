#version 450

layout(binding = 0)  uniform sampler2D IrradianceAtlas;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    mat4 view;
    mat4 proj;
    mat4 model;

    int   AtlasWidthSize;  
    int   ProbeSideLength; 
    int   GutterSize;      
    int   NumRays;
    int   ShowDebugStatus;
}pc;

layout(location = 0) in flat int  ProbeIndex;
layout(location = 1) in vec2 TexCoord;
layout(location = 2) in  flat int ProbeStatus;

void main() {
   
  bool ShowProbeStatus = bool(pc.ShowDebugStatus);
  if(ShowProbeStatus){    
       
       if(ProbeStatus == 0){
           outColor = vec4(0,1,0,1);
        return;
       }

        if(ProbeStatus == 1){
           outColor = vec4(0,0,1,1);
        return;
       }

        if(ProbeStatus == 2){
           outColor = vec4(1,0,0,1);
        return;
       }
  } else{
  
       int ProbeSideWithGutter = pc.ProbeSideLength +  pc.GutterSize;
       int ProbesPerRow        = pc.AtlasWidthSize  /  ProbeSideWithGutter; // How many Probes can be fit in a row?
       
       // Convert integer sizes to float for UV calculations
       float FAtlasWidth       = float(pc.AtlasWidthSize);
       float FAtlasHeight      = FAtlasWidth;
       float FProbeSideLength  = float(pc.ProbeSideLength);
       float FGutterSize       = float(pc.GutterSize);
       float FHalfGutter       = FGutterSize / 2.0;
       
       ///Find the  general offset (What Index in the atlas are we in ?) this is the pixel of the Top left corner of the probe
       float CellOffset_X = (float((ProbeIndex % ProbesPerRow) * ProbeSideWithGutter)) / FAtlasWidth; // normliase to UV space
       float CellOffset_Y = (float((ProbeIndex / ProbesPerRow) * ProbeSideWithGutter)) / FAtlasHeight; // normliase to UV space
       
       ///(skip half the gutter)
       float GutterOffsetX = FHalfGutter / FAtlasWidth;
       float GutterOffsetY = FHalfGutter / FAtlasHeight;
       
       //Add both the offset together
       vec2 DataOffset;
       DataOffset.x = CellOffset_X + GutterOffsetX;
       DataOffset.y = CellOffset_Y + GutterOffsetY;
       
       //Scale the texture coordinate so that it can correctly sample ther probe data
       vec2 DataScale;
       DataScale.x = FProbeSideLength / FAtlasWidth;
       DataScale.y = FProbeSideLength / FAtlasHeight;
       
       //Combine them togetherr
       vec2 tc = (TexCoord * DataScale) + DataOffset;
       
       vec4 probe_data = texture(IrradianceAtlas, tc);
       outColor = vec4(probe_data.xyz, 1.0);
  }
}