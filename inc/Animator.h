#ifndef ANIMATOR_H
#define ANIMATOR_H

namespace osp {

class Animator {
public:
	static Animator * New();
	void SetCurrentTimestep(unsigned int _timestep);
	void SetRefreshInterval(float _refreshInterval);
	void Update(float _elapsedTime);
	void TogglePause();
	void SetNumTimesteps(unsigned int _numTimesteps);
	void IncTimestep();
	void DecTimestep();

	unsigned int CurrentTimestep() const { return currentTimestep_; }
private:
	Animator();
	Animator(const Animator&) { }

  unsigned int numTimesteps_;
	unsigned int currentTimestep_;
	bool paused_;
	float elapsedTime_;
	float refreshInterval_;
};

}

#endif

