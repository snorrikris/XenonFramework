module;

export module Demo.ViewManager_IF;

export import Xe.ViewManagerIF;

export class CViewManagerDemoIF : public virtual CXeViewManagerIF
{
public:
	virtual dsid_t GetNextNewDataSourceId() = 0;
};

