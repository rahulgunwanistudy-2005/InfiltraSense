#include "../sketch.ino"

#include <cassert>

float fluidLevel = 0;

void step(float fluid = 0, float motion = 0, float contact = 1,
          float shift = 0, float expansion = 0, float temp = 36.5) {
  fakeTime += 100;
  const float t = fakeTime / 1000.f;
  fluidLevel += (fluid - fluidLevel) * .065f;
  const float wave = sinf(t * 11.7f);
  const float noise = .12f * sinf(t * 2.3f);

  x = {++lastSeq,
       1000 * (1 - .0026f * fluidLevel - .00035f * shift + .13f * motion * wave) + noise,
       650 * (1 - .0013f * fluidLevel - .00012f * shift + .07f * motion * wave) + noise,
       .145f * fluidLevel + .10f * shift + expansion + 8 * motion * wave + noise,
       temp + .004f * fluidLevel + .01f * sinf(t * .7f),
       motion,
       contact};

  if (contact < .5f) {
    x.lf = 2800 + 600 * wave;
    x.hf = 1900 + 350 * wave;
  }
  process(.1f);
}

void run(int count, float fluid = 0, float motion = 0, float contact = 1,
         float shift = 0, float expansion = 0, float temp = 36.5) {
  while (count--) step(fluid, motion, contact, shift, expansion, temp);
}

int main() {
  run(70);
  assert(state == NORMAL && baseline && !latched);
  GraphPoint normalGraph = graphPoint();
  assert(normalGraph.lf < 100 && normalGraph.hf < 100 &&
         normalGraph.strain < 100 && normalGraph.motion < 100 &&
         normalGraph.temp < 100 && normalGraph.check == 0);
  serialOutput.clear();
  plotGraph();
  assert(serialOutput.find("LF_Z:") != std::string::npos &&
         serialOutput.find("HF_Z:") != std::string::npos &&
         serialOutput.find("Strain:") != std::string::npos &&
         serialOutput.find("Motion:") != std::string::npos &&
         serialOutput.find("Temperature:") != std::string::npos &&
         serialOutput.find("Threshold:100.0") != std::string::npos &&
         serialOutput.find("Candidate:") != std::string::npos &&
         serialOutput.find("CHECK_IV:") != std::string::npos);
  puts("PASS boot / normal");

  requestReacquisition();
  assert(!baseline && quietTime == 0 && baselineTime == 0 && abstain);
  run(9);
  assert(quietTime < 1 && baselineTime == 0);
  run(42);
  assert(baseline && state == NORMAL);
  puts("PASS manual reacquisition enforces settling");

  run(80, 0, .85f);
  assert(state == WATCH && abstain && !latched && persist == 0);
  assert(graphPoint().motion >= 100 && !candidateEvidence);
  puts("PASS motion rejection");

  run(30);
  assert(state == NORMAL);
  run(30, 0, 0, .15f);
  assert(state == INVALID && !baseline && !latched);
  puts("PASS contact rejection");

  run(20);
  assert(!baseline && state == WATCH);
  run(50);
  assert(baseline && state == NORMAL);
  puts("PASS guarded reacquisition");

  run(100, 0, 0, 1, 85);
  assert(state == WATCH && !latched);
  puts("PASS shift / no false escalation");

  run(20);
  run(20, 85);
  assert(!latched);
  run(100, 85);
  assert(state == CHECK && latched);
  GraphPoint alertGraph = graphPoint();
  assert(alertGraph.lf >= 100 && alertGraph.hf >= 100 &&
         alertGraph.strain >= 100 && alertGraph.candidate > 100 &&
         alertGraph.check > 100);
  puts("PASS fluid / persistence / alert");

  acknowledged = true;
  run(40, 85);
  assert(latched);
  puts("PASS acknowledgement preserves ongoing alert");

  run(20, 85, 0, .15f);
  assert(state == INVALID && latched);
  puts("PASS contact fault retains alert");

  run(80, 85);
  assert(!baseline && latched);
  puts("PASS abnormal site cannot rebaseline");

  run(200);
  assert(state == NORMAL && !latched && baseline);
  puts("PASS washout / reacquisition / acknowledged clear");

  run(80, 0, 0, 1, 0, 10);
  assert(state == WATCH && !latched);
  puts("PASS independent strain no escalation");

  run(30);
  run(80, 0, 0, 1, 0, 0, 38);
  assert(state == WATCH && !latched);
  assert(graphPoint().temp >= 100 && !candidateEvidence);
  puts("PASS temperature alone no escalation");

  run(30);
  run(30, 85);
  assert(!latched);
  run(20, 85, .85f);
  assert(persist == 0 && !latched);
  puts("PASS interrupted evidence resets persistence");

  lastFrame = fakeTime;
  fakeTime += 800;
  loop();
  assert(state == INVALID && !baseline && abstain);
  puts("PASS sensor stream timeout");
}
