module;

#include <chrono>
#include <thread>

export module Xe.ChronoTimer;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

export class ChronoTimer
{
public:
	ChronoTimer() { m_timer = std::chrono::steady_clock::now(); }
	int64_t Elapsed_mS()
	{
		auto end = std::chrono::steady_clock::now();
		return (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(end - m_timer).count();
	}
	int64_t Elapsed_microSec()
	{
		auto end = std::chrono::steady_clock::now();
		return (int64_t)std::chrono::duration_cast<std::chrono::microseconds>(end - m_timer).count();
	}
	int64_t Elapsed_Sec()
	{
		auto end = std::chrono::steady_clock::now();
		return (int64_t)std::chrono::duration_cast<std::chrono::seconds>(end - m_timer).count();
	}
	void Reset()
	{
		m_timer = std::chrono::steady_clock::now();
	}
	static void Sleep_mS(int mS)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(mS));
	}
	static void Sleep_microSec(int uS)
	{
		std::this_thread::sleep_for(std::chrono::microseconds(uS));
	}
protected:
	std::chrono::time_point<std::chrono::steady_clock> m_timer;
};

export class TimeoutTimer
{
	ChronoTimer m_timeoutTimer;
	int64_t m_timeout_mS = LLONG_MAX;
	bool m_isTimeout = false;

public:
	void ResetTimeoutTimer(int64_t timeout_mS = LLONG_MAX)
	{
		m_timeoutTimer.Reset();
		m_timeout_mS = timeout_mS;
		m_isTimeout = false;
	}

	inline bool IsTimeoutElapsed()
	{
		if (m_timeoutTimer.Elapsed_mS() > m_timeout_mS)
		{
			m_isTimeout = true;
		}
		return m_isTimeout;
	}

	inline bool IsTimeoutFlagSet() const { return m_isTimeout; }
};

