#version 410

// 각 라이트 유형에 필요한 유니폼 변수를 관리하는 구조체를 정의함. (구조체는 ts interface 랑 비슷한 개념. cpp 에서 쓰던 구조체와 문법이 거의 동일함.)
struct DirectionalLight {
  vec3 direction;
  vec3 color;
};

struct PointLight {
  vec3 position;
  vec3 color;
  float radius;
};

struct SpotLight {
  vec3 position;
  vec3 direction;
  vec3 color;
  float cutoff;
};

// 각 라이트 유형별로 생성할 구조체배열의 크기를 매크로 상수로 정의해 둠. (cpp 에서 쓰던 매크로 상수와 동일함.)
#define NUM_DIR_LIGHTS 1
#define NUM_POINT_LIGHTS 2
#define NUM_SPOT_LIGHTS 2

// c++ 미리 계산된 후 받아온 조명연산에 필요한 유니폼 변수들

// 각 라이트 유형별 구조체배열을 유니폼 변수로 정의함. (한 셰이더에서 여러 개의 라이트(멀티라이트)를 계산하기 위해 구조체가 담긴 배열을 생성한 것)
uniform DirectionalLight directionalLights[NUM_DIR_LIGHTS];
uniform PointLight pointLights[NUM_POINT_LIGHTS];
uniform SpotLight spotLights[NUM_SPOT_LIGHTS];

uniform vec3 cameraPos; // 각 프래그먼트 -> 카메라 방향의 벡터 (이하 '뷰 벡터' 또는 '카메라 벡터') 계산에 필요한 카메라 월드공간 좌표
uniform vec3 ambientCol; // 앰비언트 라이트(환경광 또는 글로벌 조명(전역 조명))의 색상 
uniform sampler2D diffuseTex; // 디퓨즈 라이팅 계산에 사용할 텍스쳐를 담는 변수
uniform sampler2D specTex; // 스펙큘러 라이팅 계산에 사용할 텍스쳐를 담는 변수
uniform sampler2D nrmTex; // 노말 매핑에 사용할 노말맵 텍스쳐를 담는 변수

uniform samplerCube envMap; // 환경광 반사를 물 셰이더에 적용하기 위해 필요한 큐브맵(환경맵)을 전달받는 유니폼 변수

in vec3 fragNrm; // 버텍스 셰이더에서 받아온 shield 모델의 (월드공간) 노멀벡터가 보간되어 들어온 값
in vec3 fragWorldPos; // 버텍스 셰이더에서 받아온 shield 모델의 월드공간 위치 좌표가 보간되어 들어온 값
in vec2 fragUV; // 라이팅 계산에 사용할 텍스쳐들을 샘플링하기 위해 shield 모델의 버텍스의 uv좌표들을 보간하여 들어온 값
in mat3 TBN; // 노말맵 텍스쳐에서 샘플링한 탄젠트 공간의 노말벡터를 월드공간으로 변환하기 위해, 버텍스 셰이더에서 각 버텍스마다 계산한 뒤, 보간되어 들어온 TBN 행렬

out vec4 outCol; // 최종 출력할 색상을 계산하여 다음 파이프라인으로 넘겨줄 변수

// 디렉셔널 라이트, 포인트 라이트, 스포트 라이트 등 어떤 라이트 유형이라도 가져다 쓸 수 있도록 블린-퐁 라이트의 각 요소들을 계산하는 별도 함수를 추출함 -> 일종의 리팩토링
// 디퓨즈 라이팅 계산 (노멀벡터와 조명벡터를 내적)
float diffuse(vec3 lightDir, vec3 normal) {
  float diffAmt = max(0.0, dot(normal, lightDir)); // 정규화된 노멀벡터와 조명벡터의 내적값을 구한 뒤, max() 함수로 음수인 내적값 제거.
  return diffAmt;
}

// Blinn-Phong 공식에서의 스펙큘러 라이팅 계산
float specular(vec3 lightDir, vec3 viewDir, vec3 normal, float shininess) {
  vec3 halfVec = normalize(viewDir + lightDir); // 뷰 벡터와 조명벡터 사이의 하프벡터를 구함
  float specAmt = max(0.0, dot(halfVec, normal)); // 하프벡터와 노멀벡터의 내적값을 구한 뒤, max() 함수로 음수값 제거
  return pow(specAmt, shininess); // 퐁 반사모델에서와 동일한 스펙큘러 하이라이트를 얻으려면, 퐁 반사모델에서 사용했던 광택값의 2~4배 값을 거듭제곱해야 함. 따라서 0.5의 4배인 2를 광택값으로 사용함.
}

void main(){
  /**
    멀티라이트 셰이더에서는 여러 개의 라이트 유형들을
    하나의 셰이더 내에서 계산하기 때문에,

    한 번만 계산한 뒤 공통으로 재사용할 값들은
    모두 main() 함수의 앞부분에 미리 다 계산해놓는 것이 좋음.
  */
  // vec3 normal = normalize(fragNrm); // 프래그먼트 셰이더에서 보간된 노멀벡터는 길이가 1로 보존되지 않으므로, 연산에 사용하기 전 다시 정규화해줘야 함.
  // 버텍스 셰이더에서 받아온 노말벡터(fragNrm)를 보간하지 않고, 노말맵 텍스쳐에서 샘플링한 노말벡터를 TBN 행렬로 곱해 월드공간으로 변환한 후 사용할 것임.
  vec3 normal = texture(nrmTex, fragUV).rgb; // 노말맵 텍스쳐에서 텍셀값을 샘플링한 뒤, vec3 노말벡터 자리에 할당함.

  // 위의 샘플링한 노말벡터는 어디까지나 텍스쳐의 텍셀값, 즉 색상값이므로 범위가 0 ~ 1까지 밖에 표현이 안됨. 
  // 그런데, 탄젠트 공간의 노말벡터는 실제로 -1 ~ 1 사이의 정규화된 좌표계를 사용하고 있고, 음의 방향으로도 벡터를 표현할 수 있어야 하기 때문에
  // 0 ~ 1 사이의 컴포넌트 범위를 -1 ~ 1 사이로 맵핑한 뒤 정규화한 것.
  normal = normalize(normal * 2.0 - 1.0);
  normal = normalize(TBN * normal); // 컴포넌트의 값 범위를 바로잡은 노말벡터를 TBN 행렬과 곱해줌으로써, 탄젠트공간 -> 월드공간 으로 변환을 수행한 뒤, 길이를 1로 다시 정규화함.
  // 여기까지 해야 노말맵에서 샘플링해온 노말벡터는 조명계산에 써먹을 수 있는 상태가 되었고, 이후의 계산은 원래 하던 blinn-phong 계산과 동일하게 수행하면 됨.

  vec3 viewDir = normalize(cameraPos - fragWorldPos); // 카메라의 월드공간 좌표 - 각 프래그먼트 월드공간 좌표를 빼서 각 프래그먼트 -> 카메라 방향의 벡터인 뷰 벡터 계산

  vec3 envSample = texture(envMap, reflect(-viewDir, normal)).xyz; // 큐브맵 텍스쳐로부터 방향벡터를 사용해 샘플링한 텍셀값
  float specMask = texture(specTex, fragUV).x; // c++ 에서 전달해준 스펙큘러 하이라이트 색상 대신, 스펙큘러 맵에서 샘플링한 텍셀값을 사용할거임. 
  vec3 diffuseColor = texture(diffuseTex, fragUV).xyz; // 물체의 원색상은 c++ 에서 전달해준 색상값 대신, 디퓨즈 텍스쳐에서 샘플링한 텍셀값을 사용할거임.

  vec3 finalColor = vec3(0.0, 0.0, 0.0); // 이후 계산되는 각 유형별 라이트값은 여기에 누적계산 할거임.

  // 각 라이트 유형별로 for 반복문을 돌면서 구조체배열 안에 담긴 값들을 가져와서 순차적으로 라이팅 계산을 해줄거임.
  // 디렉셔널 라이트 구조체배열 반복문
  for(int i = 0; i < NUM_DIR_LIGHTS; ++i) {
    DirectionalLight light = directionalLights[i]; // 디렉셔널 라이트 구조체배열에 저장한 디렉셔널 라이트 구조체들을 순차적으로 꺼내와서 계산함.
    vec3 sceneLight = mix(light.color, envSample + light.color * 0.5, 0.5); // 환경맵 반사 및 cpp에서 전달해 준 조명색상이 반영된 sceneLight 변수 계산

    float diffAmt = diffuse(light.direction, normal); // 별도로 추출한 함수로부터 디퓨즈 라이팅 값 리턴받음.
    float specAmt = specular(light.direction, viewDir, normal, 4.0) * specMask; // 별도로 추출한 함수로부터 스펙큘러 라이팅 값 리턴받음.

    vec3 specCol = specMask * sceneLight * specAmt; // 최종 스펙큘러 라이트 색상값을 계산함. (specMask 를 한 번 더 곱해준 이유는, sceneLight 가 specMask 영역만큼 적용될 수 있도록 하기 위함.)

    finalColor += diffuseColor * diffAmt * sceneLight; // '물체의 원색상 * sceneLight * 디퓨즈 라이트값' 을 곱해 디퓨즈 라이트 색상값을 구한 뒤 최종 색상에 더해줌.
    finalColor += specCol * light.color; // 최종 스펙큘러 라이트 색상값에 cpp 에서 받아온 디렉셔널 라이트 색상값을 곱해서 최종최종 스펙큘러 라이트 색상값을 구하여 최종 색상에 더해줌.
  }

  // 포인트라이트 구조체배열 반복문
  for(int i = 0; i < NUM_POINT_LIGHTS; ++i) {
    PointLight light = pointLights[i]; // 포인트라이트 구조체배열에 저장한 포인트 라이트 구조체들을 순차적으로 꺼내와서 계산함.
    vec3 sceneLight = mix(light.color, envSample + light.color * 0.5, 0.5); // 환경맵 반사 및 cpp에서 전달해 준 조명색상이 반영된 sceneLight 변수 계산

    // 포인트 라이트 방향벡터 및 감쇄값 계산
    vec3 toLight = light.position - fragWorldPos; // 포인트라이트 월드공간 위치 ~ 각 프래그먼트 월드공간 위치까지의 벡터 계산
    vec3 lightDir = normalize(toLight); // 위에서 구한 각 프래그먼트에 도달하는 포인트라이트 방향벡터의 길이를 1로 맞춰서 방향벡터 구함.
    float distToLight = length(toLight); // 정규화되지 않은 각 프래그먼트에 도달하는 포인트라이트 벡터의 길이값, 즉, 각 프래그먼트에서 조명까지의 거리값을 구해놓음.
    float falloff = 1.0 - (distToLight / light.radius); // 각 프래그먼트에서 조명까지의 거리값을 포인트라이트 조명의 반경(최대범위)로 나눈 뒤, 1에서 빼줌으로써 감쇄값을 계산함.
    // 왜냐하면, 프래그먼트가 포인트라이트에 가까울수록 (distToLight / lightRadius) 는 0에 가깝겠지만, specAmt, diffAmt 에 곱해주는 감쇄값인 falloff 는 가까울수록 1에 가까워야 할테니
    // 1에서 빼줘서 값을 뒤집어준 것임!

    // 디렉셔널 라이트와 다르게 디퓨즈 라이팅값과 스펙큘러 라이팅값 계산 이후 위에서 구한 '감쇄값' 을 추가로 곱해줘야 함!
    float diffAmt = diffuse(lightDir, normal) * falloff; // 별도로 추출한 함수로부터 디퓨즈 라이팅 값 리턴받음.
    float specAmt = specular(lightDir, viewDir, normal, 4.0) * specMask * falloff; // 별도로 추출한 함수로부터 스펙큘러 라이팅 값 리턴받음.

    vec3 specCol = specMask * sceneLight * specAmt; // 최종 스펙큘러 라이트 색상값을 계산함. (specMask 를 한 번 더 곱해준 이유는, sceneLight 가 specMask 영역만큼 적용될 수 있도록 하기 위함.)

    finalColor += diffuseColor * diffAmt * sceneLight; // '물체의 원색상 * sceneLight * 디퓨즈 라이트값' 을 곱해 디퓨즈 라이트 색상값을 구한 뒤 최종 색상에 더해줌.
    finalColor += specCol; // 최종 스펙큘러 라이트 색상값을 최종 색상에 더해줌.
  }

  for(int i = 0; i < NUM_SPOT_LIGHTS; ++i) {
    SpotLight light = spotLights[i]; // 스포트라이트 구조체배열에 저장한 스포트라이트 구조체들을 순차적으로 꺼내와서 계산함.
    vec3 sceneLight = mix(light.color, envSample + light.color * 0.5, 0.5); // 환경맵 반사 및 cpp에서 전달해 준 조명색상이 반영된 sceneLight 변수 계산

    // 스포트라이트로 향하는 방향벡터 및 감쇄값 계산
    vec3 toLight = light.position - fragWorldPos; // 각 프래그먼트 월드공간 위치 -> 스포트라이트 월드공간 위치까지의 벡터 계산
    vec3 lightDir = normalize(toLight); // 위에서 구한 각 프래그먼트에서 스포트라이트로 향하는 방향벡터의 길이를 1로 맞춰서 방향벡터 구함.
    float angle = dot(light.direction, -lightDir); // 각 프래그먼트에서 스포트라이트로 향하는 방향벡터와 원뿔의 정가운데 벡터를 내적해서 두 벡터 사이의 각도의 cos값을 구함.
    // 이때, light.direction 은 조명에서 나오는 방향이지만, lightDir 은 조명으로 향하는 방향이라서, -lightDir 로 음수화하여 방향을 light.direction 과 맞춰줌. (그래야 내적계산 결과가 정확해짐.)

    // 내적결과값인 angle 이 cutoff 값(원뿔 최대 범위 각도의 cos값)보다 커야만 감쇄를 1로 지정해서 조명값이 밝게 나오도록 하고, 그렇지 않으면 0으로 지정해서 어둡게 나오도록 함.
    // 뭔가 '최대범위'라는 말 때문에 cutoff 보다 작은 값을 1로 찍어줘야 하는 거 아닌지 생각할수도 있지만,
    // 내적결과값이 1이면 두 벡터가 일치하는, 즉 두 벡터사이의 각도가 0도라는 뜻이므로, 
    // 특정 각도의 cos값보다 클수록, 두 벡터사이의 각도가 0도에 가까워질테니, 당연히 원뿔 범위 영역안에 들어오게 되겠지!
    float falloff = (angle > light.cutoff) ? 1.0 : 0.0; // 쉐이더에서도 삼항연산자를 쓸 수 있음!

    // 디렉셔널 라이트와 다르게 디퓨즈 라이팅값과 스펙큘러 라이팅값 계산 이후 위에서 구한 '감쇄값' 을 추가로 곱해줘야 함!
    float diffAmt = diffuse(lightDir, normal) * falloff; // 별도로 추출한 함수로부터 디퓨즈 라이팅 값 리턴받음.
    float specAmt = specular(lightDir, viewDir, normal, 4.0) * specMask * falloff; // 별도로 추출한 함수로부터 스펙큘러 라이팅 값 리턴받음.

    vec3 specCol = specMask * sceneLight * specAmt; // 최종 스펙큘러 라이트 색상값을 계산함. (specMask 를 한 번 더 곱해준 이유는, sceneLight 가 specMask 영역만큼 적용될 수 있도록 하기 위함.)

    finalColor += diffuseColor * diffAmt * sceneLight; // '물체의 원색상 * sceneLight * 디퓨즈 라이트값' 을 곱해 디퓨즈 라이트 색상값을 구한 뒤 최종 색상에 더해줌.
    finalColor += specCol; // 최종 스펙큘러 라이트 색상값을 최종 색상에 더해줌.
  }

  outCol = vec4(finalColor + ambientCol, 1.0); // '각 라이트 유형별 라이팅값이 누적계산된 최종색상값 + 앰비언트 라이트 색상값' 을 합쳐서 최종최종 색상값 결정
}

/*
  texture()

  원래 glsl 내장함수로 텍스쳐 샘플링할 때
  texture2D() 함수를 사용했었는데,
  현재 410 버전에서 사용하면 에러가 남.

  아무래도 410 버전의 glsl 은 문법이 변경된 거 같음.
*/
