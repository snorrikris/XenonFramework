module;
#include <string>

export module Xe.DefData;

// Wrapper class for data source id (for type safety).
export class  dsid_t
{
	uint32_t m_ds_id;
public:
	constexpr static dsid_t MakeDSID(uint32_t id) { dsid_t dsid; dsid.set(id); return dsid; }
	constexpr dsid_t() : m_ds_id(0) {}
	constexpr bool operator==(const dsid_t& rhs) const { return m_ds_id == rhs.m_ds_id; }
	constexpr bool operator!=(const dsid_t& rhs) const { return !operator==(rhs); }
	constexpr bool operator<(const dsid_t& rhs) const { return m_ds_id < rhs.m_ds_id; }
	constexpr bool is_valid() const { return m_ds_id > 0 && m_ds_id < 0xFFFFFFFF; }
	constexpr bool is_zero() const { return m_ds_id == 0; }
	constexpr bool is_max() const { return m_ds_id == 0xFFFFFFFF; }
	constexpr void reset() { m_ds_id = 0; }
	constexpr uint32_t get() const { return m_ds_id; }
	constexpr void set(uint32_t dsid) { m_ds_id = dsid; }
	std::wstring to_wstring() const { return std::to_wstring(m_ds_id); }
	std::string to_string() const { return std::to_string(m_ds_id); }
	void from_string(const std::string& str)
	{
		try
		{
			m_ds_id = (uint32_t)std::stol(str);
		}
		catch (const std::exception&)
		{
			m_ds_id = 0;
		}
	}
	void from_wstring(const std::wstring& str)
	{
		try
		{
			m_ds_id = (uint32_t)std::stol(str);
		}
		catch (const std::exception&)
		{
			m_ds_id = 0;
		}
	}
};

export enum class ETABVIEWID { eAnyTabVw = 0, ePrimaryTabVw, eSecondaryTabVw };

export struct CreateViewParams
{
	CreateViewParams() = default;
	CreateViewParams(ETABVIEWID tabId) : eTabVwId(tabId) {}
	ETABVIEWID eTabVwId = ETABVIEWID::eAnyTabVw;
	dsid_t insertBeforeDSid = dsid_t::MakeDSID(0xFFFFFFFF);	// DS id of view (in tab) to insert before of, 0 = insert at front, 0xFFFFFFFF = insert at back.
	bool makeFirstOpenedCurrentView = true;
	bool makeThisCurrentView = false;		// true if 'this' view should be made current view.
	bool gotoLastRowInGrid = false;
	bool isTabPinned = false;
};

