# 2025 Capstone – 머신러닝의 암호학적 활용 방안 연구 개발

> Tree Parity Machine(TPM) 기반 Neural Key Exchange PoC

본 프로젝트는 하단 참고자료(References)에 수록된 논문을 참고하여 구현되었습니다.  
이 저장소는 캡스톤 디자인 주제인 「머신러닝의 암호학적 활용 방안 연구 개발」의 결과물로, **Tree Parity Machine(TPM)** 기반 **신경망 키 교환(Neural Key Exchange)** PoC를 구현하고 학습 규칙별 동기화 특성을 비교·정리합니다.  
기존의 수학적 난제 기반 키 교환(DH 등)과 달리, 본 연구는 **두 노드가 동일한 키(가중치)를 학습을 통해 동기화**하는 과정을 구현하는 데 초점을 둡니다.

---

## 팀 정보
- **지도교수**: 김태근 (고려대학교)
- **PL**: 이상수 (ETRI)
- **팀원**: 임건형(팀장), 양채한, 김주빈, 김태흔
- **소속**: 고려대학교 인공지능사이버보안학과

---

## 개발 환경
- OS: Ubuntu 22.04.5 LTS
- Language: Python 3.12.x, C

---

## 프로젝트 구성(Architecture)
두 노드가 TCP로 통신하며, 각 노드는 다음 모듈로 구성됩니다.
- **TCP Module**: 노드 간 TCP 통신/세션 관리
- **TPM Module(ML)**: TPM 기반 Key Generation(동기화) 연산 및 업데이트 로직

---

## TPM 기반 키 교환 동작 흐름(요약)
1. 각 노드는 **초기 가중치(Weight)를 랜덤/비밀로 설정**합니다.
2. 두 노드는 **공개 입력값**을 생성/교환합니다.
3. 입력과 가중치로 TPM 연산을 수행하고, **조건을 만족할 때만 가중치를 업데이트**합니다.
4. 출력값 비교/교환을 반복하며, 일정 조건에서 **가중치가 동기화되면 키 합의 완료**로 판단합니다.
5. 업데이트 조건 불충족이 연속 발생하는 등 특정 조건에서는 통신을 종료합니다.

### 주요 하이퍼파라미터(예시)
> 논문 예시 설정: **K=3, N=4, L=3**
- **K**: hidden unit 개수
- **N**: 각 hidden unit의 입력 개수
- **L**: weight 범위(예: L=3)
- 최종 키는 **가중치 배열(Weight Vector)** 형태로 표현됩니다.

---

## 구현한 학습 규칙(Learning Rules)
본 저장소는 TPM 동기화에 사용되는 대표 학습 규칙을 구현하고 비교합니다.
- **Random Walk**: 두 값이 같을 때 해당 유닛의 weight를 작은 폭으로 이동시키는 보수적 업데이트 규칙
- **Query**: 완전 랜덤 입력 대신, 가중치/특성 세기에 맞춰 조절된 입력을 사용하여 동기화 특성을 개선하는 방식
- **Anti-Hebbian**: 가중치가 한 방향으로 쏠리는 현상을 완화하도록 반대 방향 성격으로 조정하는 업데이트 규칙

---

## 실험/결과(요약)
학습 규칙별로 다음 지표 관점에서 동기화 특성과 효율을 비교합니다.
- **Sync Iterations**: 동기화까지 필요한 반복 횟수
- **Repulsive Steps**: 동기화를 방해하는 업데이트/상태 변화의 발생 양상

---

## 사용 방법(간단 안내)
> 실제 실행 방법(명령어/인자/입출력 형식)은 레포 구조 확정 후 아래 항목에 정리하는 것을 권장합니다.
- 각 알고리즘(랜덤워크/쿼리/안티-헤비안)별 구현 폴더에서 실행
- 노드 2대를 띄워 TCP로 연결 후 동기화 결과 확인

---

## 참고(References)
- Bocheng Bao, Haigang Tang, Yuanhui Su, Han Bao, Mo Chen, Quan Xu, “Two-Dimensional Discrete Bi-Neuron Hopfield Neural Network With Polyhedral Hyperchaos,” IEEE, 2024, 5907–5918.
- Di Xiao, Xiaofeng Liao, Yong Wang, “Parallel keyed hash function construction based on chaotic neural network,” Neurocomputing, 2009, 2288–2296.
- Andreas Ruttor, Wolfgang Kinzel, Ido Kanter, “Neural cryptography with queries,” Journal of Statistical Mechanics: Theory and Experiment, 2005, P01009.
- Xuguang Wu, Yiliang Han, Minqing Zhang, ShuaiShuai Zhu, Su Cui, Yuanyuan Wang, Yixuan Peng, “Pseudorandom number generators based on neural networks: a review,” (Apr 2025).
