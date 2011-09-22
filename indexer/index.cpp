#include "index.hpp"
#include "data_header.hpp"

#include "../platform/platform.hpp"


MwmValue::MwmValue(string const & name)
  : m_cont(GetPlatform().GetReader(name))
{
  m_factory.Load(m_cont);
}

void Index::GetInfo(string const & name, MwmInfo & info) const
{
  MwmValue value(name);

  feature::DataHeader const & h = value.GetHeader();
  info.m_limitRect = h.GetBounds();

  pair<int, int> const scaleR = h.GetScaleRange();
  info.m_minScale = static_cast<uint8_t>(scaleR.first);
  info.m_maxScale = static_cast<uint8_t>(scaleR.second);
}

MwmValue * Index::CreateValue(string const & name) const
{
  return new MwmValue(name);
}

void Index::DestroyValue(MwmValue * p) const
{
  delete p;
}

Index::Index()
{
}

Index::~Index()
{
  Cleanup();
}
