module;

#include "os_minimal.h"
#include <vector>
#include <atomic>
#include <functional>
#include "XeAssert.h"

export module Xe.Utils;

export struct BoolFlags64
{
	uint64_t m_flags = 0;

	bool operator==(const BoolFlags64& rhs) const { return m_flags == rhs.m_flags; }
	bool operator!=(const BoolFlags64& rhs) const { return !operator==(rhs); }

	void Set(uint8_t uIdx, bool flag)
	{
		if (flag) SetFlag(uIdx); else ResetFlag(uIdx);
	}

	void SetFlag(uint8_t uIdx)
	{
		if (uIdx < 63)
		{
			uint64_t flag = 0x00000001ULL << uIdx;
			m_flags |= flag;
		}
	}
	void ResetFlag(uint8_t uIdx)
	{
		if (uIdx < 63)
		{
			uint64_t flag = 0x00000001ULL << uIdx;
			m_flags &= ~flag;
		}
	}
	bool GetFlag(uint8_t uIdx) const
	{
		if (uIdx < 63)
		{
			uint64_t flag = 0x00000001ULL << uIdx;
			return (m_flags & flag) != 0;
		}
		return false;
	}
	void InvertFlag(uint8_t uIdx)
	{
		Set(uIdx, !GetFlag(uIdx));
	}
	void OrFlags(const BoolFlags64& rhs)
	{
		m_flags |= rhs.m_flags;
	}
	int HowManyTrue(const std::vector<uint8_t>& flagsIdx) const
	{
		int count = 0;
		for (uint8_t f : flagsIdx)
		{
			if (GetFlag(f)) { ++count; }
		}
		return count;
	}
	void ResetAll()
	{
		m_flags = 0;
	}
};

export class CXeWorkPart
{
public:
	double ratio_of_total_work = 1.0;	// = 1.0 when only one work part otherwise ratio of total.
								// Note - all work parts add up to 1.0 (for correct calculations).
	size_t estimated_total = 50;
	size_t calc_interval = 5;

	size_t items_count = 0;
	size_t next_progress_calc = calc_interval;

	CXeWorkPart() = default;
	CXeWorkPart(double ratio, size_t est_count, size_t interval)
		: ratio_of_total_work(ratio), estimated_total(est_count),
		calc_interval(interval), next_progress_calc() {}

	double CalculateProgress()
	{
		next_progress_calc += calc_interval;
		if (items_count > estimated_total)
		{
			estimated_total += calc_interval;
		}
		double progress = (double)items_count / (double)estimated_total;
		progress *= ratio_of_total_work;
		return progress;
	}
};

export class CXeCancelEvent
{
protected:
	std::atomic_bool m_isCancelled = false;

	std::vector<CXeWorkPart> m_work_parts;
	size_t m_current_part = 0;

public:
	std::atomic_uint8_t m_percent_done;

	CXeCancelEvent() = default;
	void CancelWork() { m_isCancelled = true; }
	void ResetCancelEvent()
	{
		m_isCancelled = false;
		m_work_parts.clear();
		m_current_part = 0;
		m_percent_done = 0;
	}
	bool IsCancelled() { return m_isCancelled; }

	void InitializeWorkCalculation(const std::vector<CXeWorkPart>& work_parts)
	{
		m_work_parts.clear();
		double total = 0;
		for (const CXeWorkPart& part : work_parts)
		{
			total += part.ratio_of_total_work;
			m_work_parts.push_back(part);
		}
		XeASSERT(total > 0.99 && total < 1.01);
		m_current_part = 0;
	}

	void SetWorkPart(size_t part)
	{
		if (part < m_work_parts.size())
		{
			m_current_part = part;
		}
	}

	bool CountItemAndCheckIfCancelled()
	{
		if (m_current_part < m_work_parts.size())
		{
			CXeWorkPart& part = m_work_parts[m_current_part];
			if (++part.items_count >= part.next_progress_calc)
			{
				double prev_progress = 0, progress = part.CalculateProgress();
				for (size_t i = 0; i < m_current_part; ++i)
				{
					prev_progress += m_work_parts[i].ratio_of_total_work;
				}
				m_percent_done = (uint8_t)(((prev_progress + progress) * 100) + 0.5);
			}
		}
		return m_isCancelled;
	}
};

export typedef std::function<void()> WorkThreadWorkCallbackFunc;

