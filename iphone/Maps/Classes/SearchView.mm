
#import "SearchView.h"
#import "UIKitCategories.h"
#import "MapsAppDelegate.h"
#import "LocationManager.h"
#import "Statistics.h"
#import "MapViewController.h"
#import "LocationManager.h"
#import "ToastView.h"
#import "SearchSuggestCell.h"
#import "SearchResultCell.h"
#import "SearchShowOnMapCell.h"
#import "SearchCategoryCell.h"

#include "Framework.h"

#include "../../search/result.hpp"
#include "../../search/params.hpp"

#include "../../indexer/mercator.hpp"

#include "../../platform/platform.hpp"
#include "../../platform/preferred_languages.hpp"
#include "../../platform/settings.hpp"

#include "../../geometry/angles.hpp"
#include "../../geometry/distance_on_sphere.hpp"


static NSString * GetKeyboardInputLanguage()
{
  UITextInputMode * mode = [UITextInputMode currentInputMode];
  if (mode)
    return mode.primaryLanguage;
  // Use system language as a fall-back
  return [[NSLocale preferredLanguages] firstObject];
}

@interface SearchResultsWrapper : NSObject

- (id)initWithResults:(search::Results const &)res;

- (search::Result const &)resultWithPosition:(NSInteger)position;
- (NSInteger)suggestsCount;
- (NSInteger)count;
- (BOOL)isEndMarker;
- (BOOL)isEndedNormal;

@end

@interface SearchResultsWrapper ()

@property (nonatomic) NSMutableDictionary * distances;

@end

@implementation SearchResultsWrapper
{
  search::Results m_results;
}

- (id)initWithResults:(search::Results const &)results
{
  self = [super init];

  m_results = results;

  return self;
}

- (NSMutableDictionary *)distances
{
  if (!_distances)
    _distances = [[NSMutableDictionary alloc] init];
  return _distances;
}

- (NSInteger)count
{
  return m_results.GetCount();
}

- (NSInteger)suggestsCount
{
  return m_results.GetSuggestsCount();
}

- (search::Result const &)resultWithPosition:(NSInteger)position
{
  return m_results.GetResult(position);
}

- (BOOL)isEndMarker
{
  return m_results.IsEndMarker();
}

- (BOOL)isEndedNormal
{
  return m_results.IsEndedNormal();
}

@end


typedef NS_ENUM(NSUInteger, CellType)
{
  CellTypeResult,
  CellTypeSuggest,
  CellTypeShowOnMap,
  CellTypeCategory
};


@interface SearchView () <UITextFieldDelegate, UITableViewDelegate, UITableViewDataSource, SearchBarDelegate, LocationObserver>

@property (nonatomic) UITableView * tableView;
@property (nonatomic) SolidTouchImageView * topBackgroundView;
@property (nonatomic) UILabel * emptyResultLabel;

@property (nonatomic) SearchResultsWrapper * wrapper;
@property (nonatomic) NSArray * categoriesNames;

@end

@implementation SearchView

__weak SearchView * selfPointer;

- (id)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];

  [self addSubview:self.tableView];
  [self addSubview:self.topBackgroundView];
  self.topBackgroundView.height = [self defaultTopBackgroundHeight];
  [self addSubview:self.searchBar];
  [self.tableView addSubview:self.emptyResultLabel];

  self.emptyResultLabel.center = CGPointMake(self.width / 2, 40);
  self.emptyResultLabel.hidden = YES;

  self.searchBar.midX = self.width / 2;

  [self setState:SearchViewStateHidden animated:NO withCallback:NO];
  [self addObserver:self forKeyPath:@"state" options:NSKeyValueObservingOptionNew context:nil];

  selfPointer = self;

  if ([self.tableView respondsToSelector:@selector(registerClass:forCellReuseIdentifier:)])
  {
    // only for iOS 6 and higher
    [self.tableView registerClass:[SearchCategoryCell class] forCellReuseIdentifier:[SearchCategoryCell className]];
    [self.tableView registerClass:[SearchResultCell class] forCellReuseIdentifier:[SearchResultCell className]];
    [self.tableView registerClass:[SearchSuggestCell class] forCellReuseIdentifier:[SearchSuggestCell className]];
    [self.tableView registerClass:[SearchShowOnMapCell class] forCellReuseIdentifier:[SearchShowOnMapCell className]];
  }

  [self layoutSubviews];
  self.tableView.contentOffset = CGPointMake(0, -self.topBackgroundView.height);

  return self;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
  if (object == self && [keyPath isEqualToString:@"state"])
    [self setState:self.state animated:YES withCallback:NO];
}

static BOOL keyboardLoaded = NO;

- (void)setState:(SearchViewState)state animated:(BOOL)animated withCallback:(BOOL)withCallback
{
  // Clear search results on the map when clear in the search bar on the map is pressed or when we reopen search dialog
  if ((_state == SearchViewStateResults && state == SearchViewStateHidden) || state == SearchViewStateFullscreen)
    [self clearSearchResultsMode];

  UIViewAnimationOptions const options = UIViewAnimationOptionCurveEaseInOut;
  double const damping = 0.9;
  NSTimeInterval const duration = animated ? 0.3 : 0;
  CGFloat const searchBarOffset = (state == SearchViewStateHidden) ? -self.searchBar.height : [self defaultSearchBarMinY];

  CGFloat const cancelButtonMinX = (state == SearchViewStateResults) ? self.searchBar.width - 4 : self.searchBar.cancelButton.minX;
  CGFloat const textFieldBackgroundWidth = cancelButtonMinX - self.searchBar.fieldBackgroundView.minX - 8;
  CGFloat const textFieldWidth = textFieldBackgroundWidth - 60;

  if (state == SearchViewStateFullscreen)
  {
    [[MapsAppDelegate theApp].m_locationManager start:self];

    double latitude;
    double longitude;
    bool const hasPt = [[MapsAppDelegate theApp].m_locationManager getLat:latitude Lon:longitude];
    GetFramework().PrepareSearch(hasPt, latitude, longitude);

    if (keyboardLoaded)
      [self.searchBar.textField becomeFirstResponder];
    [UIView animateWithDuration:0.25 delay:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
      self.tableView.alpha = 1;
    } completion:nil];
    [UIView animateWithDuration:duration delay:0 damping:damping initialVelocity:0 options:options animations:^{
      self.searchBar.cancelButton.alpha = 1;
      self.topBackgroundView.minY = 0;
      self.topBackgroundView.height = [self defaultTopBackgroundHeight];
      self.searchBar.minY = searchBarOffset;
      self.searchBar.alpha = 1;
      self.searchBar.fieldBackgroundView.width = textFieldBackgroundWidth;
      self.searchBar.textField.width = textFieldWidth;
      [self.searchBar.clearButton setImage:[UIImage imageNamed:@"SearchBarClearButton"] forState:UIControlStateNormal];
    } completion:^(BOOL) {
      if (!keyboardLoaded)
      {
        keyboardLoaded = YES;
        [self.searchBar.textField becomeFirstResponder];
      }
    }];
  }
  else if (state == SearchViewStateResults)
  {
    [self.searchBar.textField resignFirstResponder];
    [UIView animateWithDuration:duration delay:0 damping:damping initialVelocity:0 options:options animations:^{
      self.alpha = 1;
      self.searchBar.cancelButton.alpha = 0;
      self.searchBar.alpha = 1;
      self.topBackgroundView.minY = 0;
      if ([self iPhoneInLandscape])
      {
        self.searchBar.minY = searchBarOffset - 3;
        self.topBackgroundView.height = [self defaultTopBackgroundHeight] - 10;
      }
      else
      {
        self.searchBar.minY = searchBarOffset;
        self.topBackgroundView.height = [self defaultTopBackgroundHeight];
      }
      self.tableView.alpha = 0;
      self.searchBar.fieldBackgroundView.width = textFieldBackgroundWidth;
      self.searchBar.textField.width = textFieldWidth;
      [self.searchBar.clearButton setImage:[UIImage imageNamed:@"SearchBarClearResultsButton"] forState:UIControlStateNormal];
    } completion:nil];
  }
  else if (state == SearchViewStateHidden)
  {
    [self.searchBar.textField resignFirstResponder];
    [UIView animateWithDuration:duration delay:0 damping:damping initialVelocity:0 options:options animations:^{
      self.searchBar.cancelButton.alpha = 1;
      self.searchBar.maxY = 0;
      self.searchBar.alpha = 0;
      self.topBackgroundView.maxY = 0;
      self.tableView.alpha = 0;
      self.searchBar.fieldBackgroundView.width = textFieldBackgroundWidth;
      self.searchBar.textField.width = textFieldWidth;
      [self.searchBar.clearButton setImage:[UIImage imageNamed:@"SearchBarClearButton"] forState:UIControlStateNormal];
    } completion:nil];
  }
  else if (state == SearchViewStateAlpha)
  {
    [UIView animateWithDuration:duration delay:0.1 damping:damping initialVelocity:0 options:options animations:^{
      self.alpha = 0;
    } completion:nil];
    [self.searchBar.textField resignFirstResponder];
  }
  if (withCallback)
    [self willChangeValueForKey:@"state"];
  _state = state;
  if (withCallback)
    [self didChangeValueForKey:@"state"];
  
  GetFramework().Invalidate();
}

- (void)onLocationError:(location::TLocationError)errorCode
{
  NSLog(@"Location error %i in %@", errorCode, self);
}

- (void)onLocationUpdate:(location::GpsInfo const &)info
{
  if (self.state == SearchViewStateFullscreen && ![self isShowingCategories])
  {
    search::SearchParams params;
    [self updateSearchParametersWithForce:NO outParams:params andQuery:self.searchBar.textField.text];
    params.SetPosition(info.m_latitude, info.m_longitude);
    GetFramework().Search(params);

    [self recalculateDistances];
    [self.tableView reloadData];
  }
}

- (void)recalculateDistances
{
  LocationManager * locationManager = [MapsAppDelegate theApp].m_locationManager;

  double north = -1.0;
  [locationManager getNorthRad:north];
  double lat, lon;
  if ([locationManager getLat:lat Lon:lon])
  {
    SearchResultsWrapper * wrapper = self.wrapper;
    for (NSInteger position = 0; position < [wrapper count]; position++)
    {
      search::Result const & result = [wrapper resultWithPosition:position];
      if (result.HasPoint())
      {
        string distance;
        double azimut = -1.0;
        GetFramework().GetDistanceAndAzimut(result.GetFeatureCenter(), lat, lon, north, distance, azimut);
        wrapper.distances[@(position)] = [NSString stringWithUTF8String:distance.c_str()];
      }
    }
  }
}

- (void)updateSearchParametersWithForce:(BOOL)force outParams:(search::SearchParams &)sp andQuery:(NSString *)newQuery
{
  sp.SetSearchMode(search::SearchParams::ALL);
  sp.m_query = [[newQuery precomposedStringWithCompatibilityMapping] UTF8String];
  sp.m_callback = bind(&onSearchResultCallback, _1);
  sp.SetInputLocale([GetKeyboardInputLanguage() UTF8String]);
  sp.SetForceSearch(force == YES);
}

- (void)search:(NSString *)newTextQuery
{
  self.emptyResultLabel.hidden = YES;
  [self.searchBar setSearching:YES];
  search::SearchParams sp;
  [self updateSearchParametersWithForce:YES outParams:sp andQuery:newTextQuery];
  double lat, lon;
  if ([[MapsAppDelegate theApp].m_locationManager getLat:lat Lon:lon])
    sp.SetPosition(lat, lon);
  GetFramework().Search(sp);
}

static void onSearchResultCallback(search::Results const & results)
{
  SearchResultsWrapper * wrapper = [[SearchResultsWrapper alloc] initWithResults:results];
  [selfPointer performSelectorOnMainThread:@selector(frameworkDidAddSearchResult:) withObject:wrapper waitUntilDone:NO];
}

- (void)frameworkDidAddSearchResult:(SearchResultsWrapper *)wrapper
{
  // special buben for situation when textfield is empty and we should show categories (cause 'self.wrapper' == nil)
  // but callbacks from previous non-empty search are coming and modifying 'self.wrapper'
  // so, we see previous search results with empty textfield
  if (![self.searchBar.textField.text length])
    return;
  // -----------------------------------------

  if ([wrapper isEndMarker])
  {
    if ([wrapper isEndedNormal])
    {
      [self.searchBar setSearching:NO];
      [self recalculateDistances];
      [self.tableView reloadData];
    }
  }
  else
  {
    self.emptyResultLabel.hidden = [self isShowingCategories] ? YES : ([self rowsCount] > 0);
    self.wrapper = wrapper;
    [self.tableView reloadData];
    [self.tableView setContentOffset:CGPointMake(0, -self.tableView.contentInset.top) animated:YES];
  }
}

- (void)clearSearchResultsMode
{
  Framework & framework = GetFramework();
  framework.GetBalloonManager().RemovePin();
  framework.GetBalloonManager().Dismiss();
  framework.CancelInteractiveSearch();
}

- (void)searchBarDidPressClearButton:(SearchBar *)searchBar
{
  if (self.state == SearchViewStateResults)
    [self setState:SearchViewStateHidden animated:YES withCallback:YES];
  else
    [self.searchBar.textField becomeFirstResponder];

  self.searchBar.textField.text = nil;
  [self textFieldTextChanged:nil];
}

- (void)searchBarDidPressCancelButton:(id)searchBar
{
  self.searchBar.textField.text = nil;
  [self textFieldTextChanged:nil];
  [self setState:SearchViewStateHidden animated:YES withCallback:YES];
}

// TODO: This code only for demonstration purposes and will be removed soon
- (bool)tryChangeMapStyleCmd:(NSString*)str
{
  // Hook for shell command on change map style
  bool const isDark = [str isEqualToString: @"mapstyle:dark"];
  bool const isLight = isDark ? false : [str isEqualToString: @"mapstyle:light"];
  
  if (!isDark && !isLight)
    return false;

  // close Search panel
  [self searchBarDidPressCancelButton:nil];
    
  // change map style
  MapStyle const mapStyle = isDark ? MapStyleDark : MapStyleLight;
  [[MapsAppDelegate theApp] setMapStyle: mapStyle];
    
  return true;
}

- (void)textFieldTextChanged:(id)sender
{
  NSString * newText = self.searchBar.textField.text;
  
  // TODO: This code only for demonstration purposes and will be removed soon
  if ([self tryChangeMapStyleCmd: newText])
    return;

  if ([newText length])
  {
    [self search:newText];
  }
  else
  {
    // nil wrapper means "Display Categories" mode
    self.wrapper = nil;
    [self.searchBar setSearching:NO];
    self.emptyResultLabel.hidden = YES;
    [self.tableView reloadData];
  }
}

- (void)textFieldBegin:(id)sender
{
  if (self.state == SearchViewStateResults)
    [self clearSearchResultsMode];
  [self setState:SearchViewStateFullscreen animated:YES withCallback:YES];
}

- (void)showOnMap
{
  Framework & f = GetFramework();
  if (f.ShowAllSearchResults() == 0)
  {
    NSString * secondSentence = @"";
    // Country in the viewport should be downloaded
    if (!f.IsCountryLoaded(f.GetViewportCenter()))
    {
      secondSentence = [NSString stringWithFormat:L(@"download_viewport_country_to_search"),
                        [NSString stringWithUTF8String:f.GetCountryName(f.GetViewportCenter()).c_str()]];
    }
    else
    {
      // Country in the current location should be downloaded
      CLLocation * lastLocation = [[MapsAppDelegate theApp].m_locationManager lastLocation];
      if (lastLocation && !f.IsCountryLoaded(MercatorBounds::FromLatLon(lastLocation.coordinate.latitude,
                                                                        lastLocation.coordinate.longitude)))
      {
        secondSentence = [NSString stringWithFormat:L(@"download_location_country"),
                          [NSString stringWithUTF8String:f.GetCountryName(f.GetViewportCenter()).c_str()]];
      }
    }

    NSString * message = [NSString stringWithFormat:@"%@. %@", L(@"no_search_results_found"), secondSentence];
    ToastView * toastView = [[ToastView alloc] initWithMessage:message];
    [toastView show];
  }

  search::SearchParams params;
  params.m_query = [[self.searchBar.textField.text precomposedStringWithCompatibilityMapping] UTF8String];
  params.SetInputLocale([GetKeyboardInputLanguage() UTF8String]);

  f.StartInteractiveSearch(params);

  [self setState:SearchViewStateResults animated:YES withCallback:YES];
}

- (BOOL)isShowingCategories
{
  // We are on the categories screen if wrapper is nil
  return self.wrapper == nil;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
  if (![self isShowingCategories])
  {
    [self showOnMap];
    return YES;
  }
  return NO;
}

- (CGFloat)defaultSearchBarMinY
{
  return SYSTEM_VERSION_IS_LESS_THAN(@"7") ? 3 : 20;
}

- (CGFloat)defaultTopBackgroundHeight
{
  return SYSTEM_VERSION_IS_LESS_THAN(@"7") ? 44 : 64;
}

- (BOOL)iPhoneInLandscape
{
  return self.width > self.height && !IPAD;
}

- (void)layoutSubviews
{
  if (self.state == SearchViewStateFullscreen)
    self.searchBar.minY = [self defaultSearchBarMinY];
  self.tableView.contentInset = UIEdgeInsetsMake(self.topBackgroundView.height, 0, 0, 0);
  self.tableView.scrollIndicatorInsets = self.tableView.contentInset;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell;
  CellType cellType = [self cellTypeForIndexPath:indexPath];
  switch (cellType)
  {
    case CellTypeCategory:
    {
      SearchCategoryCell * customCell = [tableView dequeueReusableCellWithIdentifier:[SearchCategoryCell className]];
      if (!customCell) // only for iOS 5
        customCell = [[SearchCategoryCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:[SearchCategoryCell className]];

      customCell.titleLabel.text = L(self.categoriesNames[indexPath.row]);
      NSString * iconName = [NSString stringWithFormat:@"CategoryIcon%@", [self.categoriesNames[indexPath.row] capitalizedString]];
      customCell.iconImageView.image = [UIImage imageNamed:iconName];
      cell = customCell;
      break;
    }
    case CellTypeShowOnMap:
    {
      SearchShowOnMapCell * customCell = [tableView dequeueReusableCellWithIdentifier:[SearchShowOnMapCell className]];
      if (!customCell) // only for iOS 5
        customCell = [[SearchShowOnMapCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:[SearchShowOnMapCell className]];

      customCell.titleLabel.text = L(@"search_on_map");
      cell = customCell;
      break;
    }
    case CellTypeResult:
    {
      SearchResultCell * customCell = [tableView dequeueReusableCellWithIdentifier:[SearchResultCell className]];
      if (!customCell) // only for iOS 5
        customCell = [[SearchResultCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:[SearchResultCell className]];

      NSInteger const position = [self searchResultPositionForIndexPath:indexPath];
      search::Result const & result = [self.wrapper resultWithPosition:position];
      NSMutableArray * ranges = [[NSMutableArray alloc] init];
      for (size_t i = 0; i < result.GetHighlightRangesCount(); i++)
      {
        pair<uint16_t, uint16_t> const & pairRange = result.GetHighlightRange(i);
        NSRange range = NSMakeRange(pairRange.first, pairRange.second);
        [ranges addObject:[NSValue valueWithRange:range]];
      }
      NSString * title = [NSString stringWithUTF8String:result.GetString()];
      [customCell setTitle:title selectedRanges:ranges];
      customCell.subtitleLabel.text = [NSString stringWithUTF8String:result.GetRegionString()];
      customCell.iconImageView.image = [UIImage imageNamed:@"SearchCellPinIcon"];
      customCell.distanceLabel.text = self.wrapper.distances[@(position)];
      customCell.typeLabel.text = [NSString stringWithUTF8String:result.GetFeatureType()];
      cell = customCell;
      break;
    }
    case CellTypeSuggest:
    {
      SearchSuggestCell * customCell = [tableView dequeueReusableCellWithIdentifier:[SearchSuggestCell className]];
      if (!customCell) // only for iOS 5
        customCell = [[SearchSuggestCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:[SearchSuggestCell className]];

      NSInteger const position = [self searchResultPositionForIndexPath:indexPath];
      search::Result const & result = [self.wrapper resultWithPosition:position];

      customCell.titleLabel.text = [NSString stringWithUTF8String:result.GetString()];
      customCell.iconImageView.image = [UIImage imageNamed:@"SearchSpot"];
      cell = customCell;
      break;
    }
  }
  return cell;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  CellType cellType = [self cellTypeForIndexPath:indexPath];
  switch (cellType)
  {
    case CellTypeCategory:
    {
      return [SearchCategoryCell cellHeight];
    }
    case CellTypeShowOnMap:
    {
      return [SearchShowOnMapCell cellHeight];
    }
    case CellTypeResult:
    {
      NSInteger const position = [self searchResultPositionForIndexPath:indexPath];
      SearchResultsWrapper * wrapper = self.wrapper;
      search::Result const & result = [wrapper resultWithPosition:position];
      NSString * title = [NSString stringWithUTF8String:result.GetString()];
      NSString * subtitle;
      NSString * type;
      if (result.GetResultType() == search::Result::RESULT_FEATURE || result.GetResultType() == search::Result::RESULT_LATLON)
      {
        subtitle = [NSString stringWithUTF8String:result.GetRegionString()];
        type = [NSString stringWithUTF8String:result.GetFeatureType()];
      }
      return [SearchResultCell cellHeightWithTitle:title type:type subtitle:subtitle distance:wrapper.distances[@(position)] viewWidth:tableView.width];
    }
    case CellTypeSuggest:
    {
      return [SearchSuggestCell cellHeight];
    }
    default:
    {
      return 0;
    }
  }
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return [self rowsCount];
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  [tableView deselectRowAtIndexPath:indexPath animated:YES];

  CellType cellType = [self cellTypeForIndexPath:indexPath];

  switch (cellType)
  {
    case CellTypeCategory:
    {
      [[Statistics instance] logEvent:@"Category Selection" withParameters:@{@"Category" : self.categoriesNames[indexPath.row]}];
      NSString * newQuery = [L(self.categoriesNames[indexPath.row]) stringByAppendingString:@" "];
      self.searchBar.textField.text = newQuery;
      [self search:newQuery];

      break;
    }
    case CellTypeShowOnMap:
    {
      [self showOnMap];
      break;
    }
    case CellTypeResult:
    {
      NSInteger const position = [self searchResultPositionForIndexPath:indexPath];
      search::Result const & result = [self.wrapper resultWithPosition:position];
      [self setState:SearchViewStateHidden animated:YES withCallback:YES];
      GetFramework().ShowSearchResult(result);
      break;
    }
    case CellTypeSuggest:
    {
      NSInteger const position = [self searchResultPositionForIndexPath:indexPath];
      search::Result const & result = [self.wrapper resultWithPosition:position];
      NSString * newQuery = [NSString stringWithUTF8String:result.GetSuggestionString()];
      self.searchBar.textField.text = newQuery;
      [self search:newQuery];

      break;
    }
  }
}

- (void)scrollViewDidScroll:(UIScrollView *)scrollView
{
  if (!scrollView.decelerating && scrollView.dragging)
    [self.searchBar.textField resignFirstResponder];
}

- (CellType)cellTypeForIndexPath:(NSIndexPath *)indexPath
{
  if ([self isShowingCategories])
  {
    return CellTypeCategory;
  }
  else
  {
    size_t const numSuggests = [self.wrapper suggestsCount];
    if (numSuggests)
      return indexPath.row < numSuggests ? CellTypeSuggest : CellTypeResult;
    else
      return indexPath.row == 0 ? CellTypeShowOnMap : CellTypeResult;
  }
}

- (NSInteger)searchResultPositionForIndexPath:(NSIndexPath *)indexPath
{
  return [self.wrapper suggestsCount] ? indexPath.row : indexPath.row - 1;
}

- (NSInteger)rowsCount
{
  if ([self isShowingCategories])
    return [self.categoriesNames count];
  else
    return [self.wrapper suggestsCount] ? [self.wrapper count] : [self.wrapper count] + 1;
}

- (NSArray *)categoriesNames
{
  if (!_categoriesNames)
    _categoriesNames = @[
        @"food",
        @"hotel",
        @"tourism",
        @"transport",
        @"fuel",
        @"shop",
        @"entertainment",
        @"atm",
        @"bank",
        @"wifi",
        @"parking",
        @"toilet",
        @"pharmacy",
        @"hospital",
        @"post",
        @"police"];
  return _categoriesNames;
}

- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event
{
  return CGRectContainsPoint(self.searchBar.frame, point) || self.state == SearchViewStateFullscreen;
}

- (UITableView *)tableView
{
  if (!_tableView)
  {
    _tableView = [[UITableView alloc] initWithFrame:self.bounds];
    _tableView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    _tableView.delegate = self;
    _tableView.dataSource = self;
    _tableView.backgroundColor = [UIColor colorWithColorCode:@"1D1F29"];
    _tableView.separatorStyle = UITableViewCellSeparatorStyleNone;
  }
  return _tableView;
}

- (SolidTouchImageView *)topBackgroundView
{
  if (!_topBackgroundView)
  {
    _topBackgroundView = [[SolidTouchImageView alloc] initWithFrame:CGRectMake(0, 0, self.width, 0)];
    if ([UIImage instancesRespondToSelector:@selector(resizableImageWithCapInsets:resizingMode:)]) // iOS 6 and higher
      _topBackgroundView.image = [[UIImage imageNamed:@"SearchViewTopBackground"] resizableImageWithCapInsets:UIEdgeInsetsMake(10, 0, 10, 0) resizingMode:UIImageResizingModeStretch];
    else // iOS 5 
      _topBackgroundView.image = [[UIImage imageNamed:@"SearchViewTopBackground"] resizableImageWithCapInsets:UIEdgeInsetsMake(10, 0, 10, 0)];
    _topBackgroundView.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    _topBackgroundView.userInteractionEnabled = YES;
  }
  return _topBackgroundView;
}

- (SearchBar *)searchBar
{
  if (!_searchBar)
  {
    _searchBar = [[SearchBar alloc] initWithFrame:CGRectMake(0, 0, self.width, 44)];
    _searchBar.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    _searchBar.textField.delegate = self;
    _searchBar.delegate = self;
    [_searchBar.textField addTarget:self action:@selector(textFieldTextChanged:) forControlEvents:UIControlEventEditingChanged];
    [_searchBar.textField addTarget:self action:@selector(textFieldBegin:) forControlEvents:UIControlEventEditingDidBegin];
  }
  return _searchBar;
}

- (UILabel *)emptyResultLabel
{
  if (!_emptyResultLabel)
  {
    _emptyResultLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, self.width, 60)];
    _emptyResultLabel.backgroundColor = [UIColor clearColor];
    _emptyResultLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:18];
    _emptyResultLabel.text = L(@"no_search_results_found");
    _emptyResultLabel.textColor = [UIColor whiteColor];
    _emptyResultLabel.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    _emptyResultLabel.textAlignment = NSTextAlignmentCenter;
  }
  return _emptyResultLabel;
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

@end