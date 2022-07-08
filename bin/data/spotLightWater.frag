#version 410

// c++ 미리 계산된 후 받아온 조명연산에 필요한 유니폼 변수들
uniform vec3 lightPos; // 조명위치 (스포트 라이트의 위치)
uniform vec3 lightConeDir; // 스포트라이트 원뿔의 정가운데 방향 조명벡터
uniform vec3 lightCol; // 조명색상 (조명강도가 곱해짐)
uniform float lightCutoff; // 스포트라이트 원뿔의 최대 각도 범위를 cos값으로 전달한 값
uniform vec3 cameraPos; // 각 프래그먼트 -> 카메라 방향의 벡터 (이하 '뷰 벡터' 또는 '카메라 벡터') 계산에 필요한 카메라 월드공간 좌표
uniform vec3 ambientCol; // 앰비언트 라이트(환경광 또는 글로벌 조명(전역 조명))의 색상 

// 물 셰이더를 만들기 위해 필요한 노말맵을 전달받는 유니폰 변수들
uniform sampler2D normTex;
uniform sampler2D normTex2; // 이 예제에서는 샘플링할 uv좌표를 달리 계산해서 서로 다른 노멀(벡터)를 구하므로, 두 번째 노멀맵이 따로 필요하진 않음.

uniform samplerCube envMap; // 환경광 반사를 물 셰이더에 적용하기 위해 필요한 큐브맵(환경맵)을 전달받는 유니폼 변수

in vec3 fragNrm; // 버텍스 셰이더에서 받아온 물 표면 모델의 (월드공간) 노멀벡터가 보간되어 들어온 값
in vec3 fragWorldPos; // 버텍스 셰이더에서 받아온 물 표면 모델의 월드공간 위치 좌표가 보간되어 들어온 값

// 각 버텍스마다 서로 다르게 계산된 uv좌표가 보간되어 들어온 값 (얘내들로 노말맵을 샘플링하므로, 서로 다른 탄젠트 공간 노멀벡터를 샘플링해올 것임)
in vec2 fragUV;
in vec2 fragUV2;

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
  return pow(specAmt, shininess); // 물 재질은 거울처럼 반사가 아주 세므로, 스펙큘러 계산 시 광택지수를 512 처럼 높게 잡아줘야 함.
}

void main(){
  // 아래의 normal, normal2 는 동일한 텍스쳐를 사용하지만,
  // 샘플링하는 uv좌표가 다르므로, 결과적으로 서로 다른 (탄젠트 공간의)노말벡터를 리턴받음.
  vec3 normal = texture(normTex, fragUV).rgb;
  normal = (normal * 2.0 - 1.0); // 노말맵에서 샘플링한 텍셀값 범위 0 ~ 1 을 탄젠트 공간의 노말벡터가 속한 좌표계 범위인 -1 ~ 1 로 맵핑함.

  vec3 normal2 = texture(normTex, fragUV2).rgb;
  normal2 = (normal2 * 2.0 - 1.0); // 노말맵에서 샘플링한 텍셀값 범위 0 ~ 1 을 탄젠트 공간의 노말벡터가 속한 좌표계 범위인 -1 ~ 1 로 맵핑함.

  normal = normalize(TBN * (normal + normal2)); // 두 탄젠트 공간의 노멀벡터를 더해서 그 사이의 하프벡터를 구하고, 그걸 TBN 행렬과 곱해 월드공간의 노말벡터로 변환함.
  // 여기까지 해야 노말맵에서 샘플링해온 노말벡터는 조명계산에 써먹을 수 있는 상태가 되었고, 이후의 계산은 원래 하던 blinn-phong 계산과 동일하게 수행하면 됨.

  vec3 viewDir = normalize(cameraPos - fragWorldPos); // 카메라의 월드공간 좌표 - 각 프래그먼트 월드공간 좌표를 빼서 각 프래그먼트 -> 카메라 방향의 벡터인 뷰 벡터 계산

  // 스포트라이트로 향하는 방향벡터 및 감쇄값 계산
  vec3 toLight = lightPos - fragWorldPos; // 각 프래그먼트 월드공간 위치 -> 스포트라이트 월드공간 위치까지의 벡터 계산
  vec3 lightDir = normalize(toLight); // 위에서 구한 각 프래그먼트에서 스포트라이트로 향하는 방향벡터의 길이를 1로 맞춰서 방향벡터 구함.
  float angle = dot(lightConeDir, -lightDir); // 각 프래그먼트에서 스포트라이트로 향하는 방향벡터와 원뿔의 정가운데 벡터를 내적해서 두 벡터 사이의 각도의 cos값을 구함.
  // 이때, lightConeDir 은 조명에서 나오는 방향이지만, lightDir 은 조명으로 향하는 방향이라서, -lightDir 로 음수화하여 방향을 lightConeDir 과 맞춰줌. (그래야 내적계산 결과가 정확해짐.)
  float falloff = 0.0; // falloff 감쇄값의 기본값은 0으로 지정해 둠. (그니까 기본값은 조명의 영향을 아예 못받는 어두운 픽셀이 찍히겠지)
  if (angle > lightCutoff) {
    // 내적결과값인 angle 이 cutoff 값(원뿔 최대 범위 각도의 cos값)보다 커야만 감쇄를 1로 지정해서 조명값이 밝게 나오도록 함.
    // 뭔가 '최대범위'라는 말 때문에 cutoff 보다 작은 값을 1로 찍어줘야 하는 거 아닌지 생각할수도 있지만,
    // 내적결과값이 1이면 두 벡터가 일치하는, 즉 두 벡터사이의 각도가 0도라는 뜻이므로, 
    // 특정 각도의 cos값보다 클수록, 두 벡터사이의 각도가 0도에 가까워질테니, 당연히 원뿔 범위 영역안에 들어오게 되겠지!
    falloff = 1.0;
  }

  float specAmt = specular(lightDir, viewDir, normal, 512.0) * falloff; // 별도로 추출한 함수로부터 스펙큘러 라이팅 값 리턴받음.
  vec3 specCol = lightCol * specAmt; // 물 셰이더에서는 스펙큘러 텍스쳐는 따로 사용 안하므로, texture(specTex, fragUV).X 뭐 이런 스펙큘러맵 샘플링값은 안곱해줘도 됨.

  float diffAmt = diffuse(lightDir, normal) * falloff; // 별도로 추출한 함수로부터 디퓨즈 라이팅 값 리턴받음.
  vec3 diffCol = texture(envMap, reflect(-viewDir, normal)).xyz * lightCol * diffAmt; // 환경맵 반사를 적용하기 위해, 물 색상을 하드코딩하지 않고, 환경맵에서 샘플링해온 텍셀값을 적용함.

  outCol = vec4(diffCol + specCol + ambientCol, 1.0);
}

/*
  환경광 반사 계산에 필요한 
  환경맵 샘플링 시 카메라 벡터와 노말벡터로 구한 반사벡터를 사용하는 이유

  원래 reflect() 함수는 조명의 반사벡터를 구하기 위해 첫 번째 인자로 
  조명벡터를 넣어주지만, 여기서는 카메라 벡터를 넣어주고 있지?

  그 이유를 추론컨데,
  환경맵 전체에서 들어오는 빛을 계산하기 어려우니,
  물 셰이더의 각각의 픽셀에서 반사되어 카메라로 들어오는
  광선들을 '역추적'해서 큐브맵을 샘플링할 방향벡터를 구한다고 생각하면 됨.

  '레이 트레이싱'과 유사한 원리를 사용하고 있으며,
  실제로 환경맵을 '레이트레이싱의 결과물을 흉내내는(simulate)'
  것이라고 설명하기도 함. 
*/