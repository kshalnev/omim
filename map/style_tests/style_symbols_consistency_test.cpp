#include "testing/testing.hpp"
#include "helpers.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/drules_include.hpp"
#include "indexer/drules_struct.pb.h"
#include "indexer/map_style_reader.hpp"

#include "base/logging.hpp"

#include "coding/parse_xml.hpp"
#include "coding/reader.hpp"

#include "std/algorithm.hpp"
#include "std/set.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

namespace
{

class SdfParsingDispatcher
{
public:
  SdfParsingDispatcher(set<string> & symbols)
      : m_symbols(symbols)
  {}

  bool Push(string const &) { return true; }
  void Pop(string const &) {}
  void CharData(string const &) {}
  void AddAttr(string const & attribute, string const & value)
  {
    if (attribute == "name")
      m_symbols.insert(value);
  }

private:
  set<string> & m_symbols;
};

set<string> GetSymbolsSetFromDrawingRule()
{
  set<string> symbols;
  drule::rules().ForEachRule([&symbols](int, int, int, drule::BaseRule const * rule)
  {
    SymbolRuleProto const * const symbol = rule->GetSymbol();
    if (nullptr != symbol && symbol->has_name())
      symbols.insert(symbol->name());
  });
  return symbols;
}

set<string> GetSymbolsSetFromResourcesFile(string const & density)
{
  set<string> symbols;
  SdfParsingDispatcher dispatcher(symbols);
  ReaderPtr<Reader> reader = GetStyleReader().GetResourceReader("symbols.sdf", density);
  ReaderSource<ReaderPtr<Reader> > source(reader);
  ParseXML(source, dispatcher);
  return symbols;
}

template <typename TIterator, typename TFn>
string Join(TIterator begin, TIterator end, TFn && fn)
{
    if (begin == end)
        return string();
    stringstream ss;
    ss << fn(*begin);
    for (++begin; begin != end; ++begin)
        ss << ", " << fn(*begin);
    return ss.str();
}

using TSymbolsMapping = map<string, map<int, string>>; // ClassifName -> Scale, SymbolName

TSymbolsMapping GetCurrentStyleSymbolsMapping()
{
    TSymbolsMapping mapping;

    string drawingRules;
    GetStyleReader().GetDrawingRulesReader().ReadAsString(drawingRules);

    ContainerProto cont;
    cont.ParseFromString(drawingRules);
    for (int ci = 0; ci < cont.cont_size(); ++ci)
    {
        ClassifElementProto const & classifElement = cont.cont(ci);
        for (int ei = 0; ei < classifElement.element_size(); ++ei)
        {
            DrawElementProto const & drawElement = classifElement.element(ei);
            if (drawElement.has_symbol())
            {
                mapping[classifElement.name()][drawElement.scale()] =
                        drawElement.symbol().name();
            }
        }
    }

    return mapping;
}

// Returns base name of icon.
// According to agreement icon name consists of 'basename-prefix'
// where prefix says about icon size, while basename identifies icon,
// prefix is optional.
string GetIconBaseName(string const & name)
{
    size_t const index = name.find_last_of('-');
    if (index == string::npos)
        return name;
    return string(name.begin(), name.begin() + index);
}

}  // namespace

UNIT_TEST(Test_SymbolsConsistency)
{
  // Tests that all symbols specified in drawing rules have corresponding symbols in resources

  bool res = true;

  string const densities[] = { "mdpi", "hdpi", "xhdpi", "xxhdpi", "6plus" };

  styles::RunForEveryMapStyle([&](MapStyle mapStyle)
  {
    set<string> const drawingRuleSymbols = GetSymbolsSetFromDrawingRule();

    for (string const & density : densities)
    {
      set<string> const resourceStyles = GetSymbolsSetFromResourcesFile(density);

      vector<string> missed;
      set_difference(drawingRuleSymbols.begin(), drawingRuleSymbols.end(),
                     resourceStyles.begin(), resourceStyles.end(),
                     back_inserter(missed));

      if (!missed.empty())
      {
        // We are interested in all set of bugs, therefore we do not stop test here but
        // continue it just keeping in res that test failed.
        LOG(LINFO, ("Symbols mismatch: style", mapStyle, ", density", density, ", missed", missed));
        res = false;
      }
    }
  });

  TEST(res, ());
}

UNIT_TEST(Test_ClassSymbolsConsistency)
{
    // Tests that symbols specified for object class are same for all zoom levels,
    // e.g. lawyer object class has 'lawyer' icon for all zoom levels.
    // Also test checks that visible scales are consecutives - without scale holes:
    // e.g. 3,4,5 - ok, but 4,5,7 - not ok.

    bool res = true;
    styles::RunForEveryMapStyle([&res](MapStyle mapStyle)
    {
        for (auto const & classifMapping : GetCurrentStyleSymbolsMapping())
        {
            auto const & scaleAndSymbolMapping = classifMapping.second;

            bool invalidSymbol = false;
            bool invalidScale = false;

            pair<int, string> prev = *scaleAndSymbolMapping.begin();
            for (auto itr = ++scaleAndSymbolMapping.begin(); itr != scaleAndSymbolMapping.end(); ++itr)
            {
                if (GetIconBaseName(prev.second) != GetIconBaseName(itr->second))
                    invalidSymbol = true;
                if ((prev.first + 1) != itr->first)
                    invalidScale = true;
                prev = *itr;
            }

            // We are interested in all bugs, therefore we do not stop test but
            // continue it just keeping in 'res' that test failed.
            if (invalidSymbol)
            {
                string icons = Join(scaleAndSymbolMapping.cbegin(), scaleAndSymbolMapping.cend(),
                                    [](pair<const int, string> const & p) { return p.second; });
                LOG(LINFO, ("Class symbol icons mismatch: style:", mapStyle, ", class:", classifMapping.first,
                            ", icons:", icons));
            }
            if (invalidScale)
            {
                string levels = Join(scaleAndSymbolMapping.cbegin(), scaleAndSymbolMapping.cend(),
                                     [](pair<const int, string> const & p) { return p.first; });
                LOG(LINFO, ("Class symbol icons mismatch: style:", mapStyle, ", class:", classifMapping.first,
                            ", levels:", levels));
            }

            res |= invalidScale || invalidSymbol;
        }
    });

    TEST(res, ());
}
