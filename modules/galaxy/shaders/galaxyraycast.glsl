/*****************************************************************************************
*                                                                                       *
* OpenSpace                                                                             *
*                                                                                       *
* Copyright (c) 2014-2019                                                               *
*                                                                                       *
* Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
* software and associated documentation files (the "Software"), to deal in the Software *
* without restriction, including without limitation the rights to use, copy, modify,    *
* merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
* permit persons to whom the Software is furnished to do so, subject to the following   *
* conditions:                                                                           *
*                                                                                       *
* The above copyright notice and this permission notice shall be included in all copies *
* or substantial portions of the Software.                                              *
*                                                                                       *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
* INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
* PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
* HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
* CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
* OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
****************************************************************************************/

uniform float maxStepSize#{id} = 0.1;
uniform vec3 aspect#{id} = vec3(1.0);
uniform float opacityCoefficient#{id} = 1.0;
uniform float absorptionMultiply#{id} = 50.0;
uniform float emissionMultiply#{id} = 1500.0;
uniform sampler3D galaxyTexture#{id};

void sample#{id}(vec3 samplePos,
  vec3 dir,
  inout vec3 accumulatedColor,
  inout vec3 accumulatedAlpha,
  inout float maxStepSize)
  {
    vec3 aspect = aspect#{id};
    maxStepSize = maxStepSize#{id} / length(dir / aspect);

    //Early ray termination on black parts of the data
    vec3 normalizedPos = (samplePos*2.0)-1.0;
    if(abs(normalizedPos.x) > 0.8 || abs(normalizedPos.y) > 0.8){
        return;
    }

    float STEP_SIZE = maxStepSize#{id}*0.5;
    //float STEP_SIZE = 1 / 256.0;

    vec3 alphaTint = vec3(0.3, 0.54, 0.85);

    vec4 sampledColor = texture(galaxyTexture#{id}, samplePos.xyz);

    // Source textures currently are square-rooted to avoid dithering in the shadows.
    // So square them back
    sampledColor = sampledColor*sampledColor;

    // fudge for the dust "spreading"
    sampledColor.a = clamp(sampledColor.a, 0.0, 1.0) * opacityCoefficient#{id};
    sampledColor.a = pow(sampledColor.a, 0.7);

    // absorption probability
    float scaled_density = sampledColor.a * STEP_SIZE * absorptionMultiply#{id};
    vec3 absorption = alphaTint * scaled_density;

    // extinction
    vec3 extinction = exp(-absorption);
    accumulatedColor.rgb *= extinction;

    // emission
    accumulatedColor.rgb += sampledColor.rgb * STEP_SIZE * emissionMultiply#{id};

    vec3 oneMinusFrontAlpha = vec3(1.0) - accumulatedAlpha;
    //accumulatedColor += oneMinusFrontAlpha * sampledColor.rgb;
    accumulatedAlpha += oneMinusFrontAlpha * sampledColor.rgb;
  }

  float stepSize#{id}(vec3 samplePos, vec3 dir) {
    return maxStepSize#{id} * length(dir * 1.0 / aspect#{id});
  }
