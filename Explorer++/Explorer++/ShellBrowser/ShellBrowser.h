// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ColumnDataRetrieval.h"
#include "Columns.h"
#include "FolderSettings.h"
#include "ServiceProvider.h"
#include "ShellChangeWatcher.h"
#include "ShellNavigator.h"
#include "SignalWrapper.h"
#include "SortModes.h"
#include "ViewModes.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellDropTargetWindow.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WinRTBaseWrapper.h"
#include "../ThirdParty/CTPL/cpl_stl.h"
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/signals2.hpp>
#include <wil/com.h>
#include <wil/resource.h>
#include <thumbcache.h>
#include <future>
#include <list>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#define WM_USER_UPDATEWINDOWS (WM_APP + 17)
#define WM_USER_FILESADDED (WM_APP + 51)

struct BasicItemInfo_t;
class CachedIcons;
struct Config;
class CoreInterface;
class FileActionHandler;
class IconFetcher;
class IconResourceLoader;
struct PreservedFolderState;
struct PreservedHistoryEntry;
class ShellNavigationController;
class TabNavigationInterface;
class WindowSubclassWrapper;

typedef struct
{
	ULARGE_INTEGER TotalFolderSize;
	ULARGE_INTEGER TotalSelectionSize;
} FolderInfo_t;

class ShellBrowser :
	public ShellDropTargetWindow<int>,
	public ShellNavigator,
	public std::enable_shared_from_this<ShellBrowser>
{
public:
	static std::shared_ptr<ShellBrowser> CreateNew(HWND hOwner, CoreInterface *coreInterface,
		TabNavigationInterface *tabNavigation, FileActionHandler *fileActionHandler,
		const FolderSettings &folderSettings, const FolderColumns *initialColumns);

	static std::shared_ptr<ShellBrowser> CreateFromPreserved(HWND hOwner,
		CoreInterface *coreInterface, TabNavigationInterface *tabNavigation,
		FileActionHandler *fileActionHandler,
		const std::vector<std::unique_ptr<PreservedHistoryEntry>> &history, int currentEntry,
		const PreservedFolderState &preservedFolderState);

	~ShellBrowser();

	HWND GetListView() const;
	FolderSettings GetFolderSettings() const;

	ShellNavigationController *GetNavigationController() const;
	boost::signals2::connection AddNavigationStartedObserver(
		const NavigationStartedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override;
	boost::signals2::connection AddNavigationCommittedObserver(
		const NavigationCommittedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override;
	boost::signals2::connection AddNavigationCompletedObserver(
		const NavigationCompletedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override;
	boost::signals2::connection AddNavigationFailedObserver(
		const NavigationFailedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override;

	/* Get/Set current state. */
	unique_pidl_absolute GetDirectoryIdl() const;
	std::wstring GetDirectory() const;
	bool GetAutoArrange() const;
	void SetAutoArrange(bool autoArrange);
	ViewMode GetViewMode() const;
	void SetViewMode(ViewMode viewMode);
	void CycleViewMode(bool cycleForward);
	SortMode GetSortMode() const;
	void SetSortMode(SortMode sortMode);
	SortMode GetGroupMode() const;
	void SetGroupMode(SortMode sortMode);
	SortDirection GetSortDirection() const;
	void SetSortDirection(SortDirection direction);
	SortDirection GetGroupSortDirection() const;
	void SetGroupSortDirection(SortDirection direction);
	bool GetShowHidden() const;
	void SetShowHidden(bool showHidden);
	int GetNumItems() const;
	int GetNumSelectedFiles() const;
	int GetNumSelectedFolders() const;
	int GetNumSelected() const;

	/* ID. */
	void SetID(int id);
	int GetId() const;

	/* Directory modification support. */
	void FilesModified(DWORD Action, const TCHAR *FileName, int EventId, int iFolderIndex);
	void DirectoryAltered();
	void SetDirMonitorId(int dirMonitorId);
	void ClearDirMonitorId();
	std::optional<int> GetDirMonitorId() const;
	int GetUniqueFolderId() const;

	/* Item information. */
	WIN32_FIND_DATA GetItemFileFindData(int index) const;
	unique_pidl_absolute GetItemCompleteIdl(int index) const;
	unique_pidl_child GetItemChildIdl(int index) const;
	std::wstring GetItemName(int index) const;
	std::wstring GetItemDisplayName(int index) const;
	std::wstring GetItemFullName(int index) const;

	void ShowPropertiesForSelectedFiles() const;

	/* Column support. */
	std::vector<Column_t> GetCurrentColumns();
	void SetCurrentColumns(const std::vector<Column_t> &columns);
	static SortMode DetermineColumnSortMode(ColumnType columnType);
	static int LookupColumnNameStringIndex(ColumnType columnType);
	static int LookupColumnDescriptionStringIndex(ColumnType columnType);

	/* Filtering. */
	std::wstring GetFilterText() const;
	void SetFilterText(std::wstring_view filter);
	bool IsFilterApplied() const;
	void SetFilterApplied(bool filter);
	bool GetFilterCaseSensitive() const;
	void SetFilterCaseSensitive(bool filterCaseSensitive);

	bool TestListViewItemAttributes(int item, SFGAOF attributes) const;
	HRESULT GetListViewSelectionAttributes(SFGAOF *attributes) const;

	void SetFileAttributesForSelection();

	void SelectItems(const std::vector<PCIDLIST_ABSOLUTE> &pidls);
	uint64_t GetTotalDirectorySize();
	uint64_t GetSelectionSize();
	int LocateFileItemIndex(const TCHAR *szFileName) const;
	bool InVirtualFolder() const;
	BOOL CanCreate() const;
	HRESULT CopySelectedItemsToClipboard(bool copy);
	void PasteShortcut();
	void StartRenamingSelectedItems();
	void DeleteSelectedItems(bool permanent);

	bool GetShowInGroups() const;
	void SetShowInGroups(bool showInGroups);

	int CALLBACK SortTemporary(LPARAM lParam1, LPARAM lParam2);

	std::vector<SortMode> GetAvailableSortModes() const;
	void ImportAllColumns(const FolderColumns &folderColumns);
	FolderColumns ExportAllColumns();
	void QueueRename(PCIDLIST_ABSOLUTE pidlItem);
	void SelectItems(const std::list<std::wstring> &PastedFileList);
	void OnDeviceChange(UINT eventType, LONG_PTR eventData);
	void OnGridlinesSettingChanged();
	void AutoSizeColumns();

	// Signals
	SignalWrapper<ShellBrowser, void()> directoryModified;
	SignalWrapper<ShellBrowser, void()> listViewSelectionChanged;
	SignalWrapper<ShellBrowser, void()> columnsChanged;

private:
	DISALLOW_COPY_AND_ASSIGN(ShellBrowser);

	struct ItemInfo_t
	{
		unique_pidl_absolute pidlComplete;
		unique_pidl_child pridl;
		WIN32_FIND_DATA wfd;
		bool isFindDataValid;
		std::wstring parsingName;
		std::wstring displayName;
		std::wstring editingName;
		int iIcon;

		/* These are only used for drives. They are
		needed for when a drive is removed from the
		system, in which case the drive name is needed
		so that the removed drive can be found. */
		BOOL bDrive;
		TCHAR szDrive[4];

		/* Used for temporary sorting in details mode (i.e.
		when items need to be rearranged). */
		int iRelativeSort;

		ItemInfo_t() : wfd({}), isFindDataValid(false), iIcon(0), bDrive(FALSE)
		{
		}
	};

	struct AlteredFile_t
	{
		TCHAR szFileName[MAX_PATH];
		DWORD dwAction;
		int iFolderIndex;
	};

	struct AwaitingAdd_t
	{
		int iItem;
		int iItemInternal;

		BOOL bPosition;
		int iAfter;
	};

	struct Added_t
	{
		TCHAR szFileName[MAX_PATH];
	};

	struct DroppedFile_t
	{
		TCHAR szFileName[MAX_PATH];
		POINT DropPoint;
	};

	struct ColumnResult_t
	{
		int itemInternalIndex;
		ColumnType columnType;
		std::wstring columnText;
	};

	struct ThumbnailResult_t
	{
		int itemInternalIndex;
		wil::unique_hbitmap bitmap;
	};

	struct InfoTipResult
	{
		int itemInternalIndex;
		std::wstring infoTip;
	};

	struct GroupInfo
	{
		std::wstring name;
		int relativeSortPosition;

		explicit GroupInfo(const std::wstring &name) : name(name), relativeSortPosition(0)
		{
		}

		GroupInfo(const std::wstring &name, int relativeSortPosition) :
			name(name),
			relativeSortPosition(relativeSortPosition)
		{
		}
	};

	struct ListViewGroup
	{
		int id;
		std::wstring name;
		int relativeSortPosition;
		int numItems;

		ListViewGroup(int id, const GroupInfo &groupInfo) :
			id(id),
			name(groupInfo.name),
			relativeSortPosition(groupInfo.relativeSortPosition),
			numItems(0)
		{
		}
	};

	enum class GroupByDateType
	{
		Created,
		Modified,
		Accessed
	};

	struct DirectoryState
	{
		unique_pidl_absolute pidlDirectory;
		std::wstring directory;
		bool virtualFolder;
		int itemIDCounter;

		/* Stores information on files that have
		been created and are awaiting insertion
		into the listview. */
		std::vector<AwaitingAdd_t> awaitingAddList;

		std::unordered_set<int> filteredItemsList;

		int numItems;
		int numFilesSelected;
		int numFoldersSelected;
		uint64_t totalDirSize;
		uint64_t fileSelectionSize;

		/* Cached folder size data. */
		mutable std::unordered_map<int, ULONGLONG> cachedFolderSizes;

		DirectoryState() :
			virtualFolder(false),
			itemIDCounter(0),
			numItems(0),
			numFilesSelected(0),
			numFoldersSelected(0),
			totalDirSize(0),
			fileSelectionSize(0)
		{
		}
	};

	// clang-format off
	using ListViewGroupSet = boost::multi_index_container<ListViewGroup,
		boost::multi_index::indexed_by<
			boost::multi_index::hashed_unique<
				boost::multi_index::member<ListViewGroup, int, &ListViewGroup::id>
			>,
			boost::multi_index::hashed_unique<
				boost::multi_index::member<ListViewGroup, std::wstring, &ListViewGroup::name>
			>
		>
	>;
	// clang-format on

	static const UINT WM_APP_COLUMN_RESULT_READY = WM_APP + 150;
	static const UINT WM_APP_THUMBNAIL_RESULT_READY = WM_APP + 151;
	static const UINT WM_APP_INFO_TIP_READY = WM_APP + 152;

	static const int THUMBNAIL_ITEM_WIDTH = 120;
	static const int THUMBNAIL_ITEM_HEIGHT = 120;

	ShellBrowser(HWND hOwner, CoreInterface *coreInterface, TabNavigationInterface *tabNavigation,
		FileActionHandler *fileActionHandler,
		const std::vector<std::unique_ptr<PreservedHistoryEntry>> &history, int currentEntry,
		const PreservedFolderState &preservedFolderState);
	ShellBrowser(HWND hOwner, CoreInterface *coreInterface, TabNavigationInterface *tabNavigation,
		FileActionHandler *fileActionHandler, const FolderSettings &folderSettings,
		const FolderColumns *initialColumns);

	static HWND CreateListView(HWND parent);
	void InitializeListView();
	int GenerateUniqueItemId();
	void MarkItemAsCut(int item, bool cut);
	void VerifySortMode();

	/* NavigatorInterface methods. */
	HRESULT Navigate(const NavigateParams &navigateParams) override;

	/* Browsing support. */
	HRESULT PerformEnumeration(const NavigateParams &navigateParams,
		std::vector<ItemInfo_t> &items);
	static HRESULT EnumerateFolder(PCIDLIST_ABSOLUTE pidlDirectory, HWND owner, bool showHidden,
		std::vector<ItemInfo_t> &items);
	static std::optional<ItemInfo_t> GetItemInformation(IShellFolder *shellFolder,
		PCIDLIST_ABSOLUTE pidlDirectory, PCITEMID_CHILD pidlChild);
	void PrepareToChangeFolders();
	void ClearPendingResults();
	void ResetFolderState();
	void StoreCurrentlySelectedItems();
	void OnEnumerationCompleted(std::vector<ItemInfo_t> &&items,
		const NavigateParams &navigateParams);
	void InsertAwaitingItems();
	BOOL IsFileFiltered(const ItemInfo_t &itemInfo) const;
	std::optional<int> AddItemInternal(IShellFolder *shellFolder, PCIDLIST_ABSOLUTE pidlDirectory,
		PCITEMID_CHILD pidlChild, int itemIndex, BOOL setPosition);
	int AddItemInternal(int itemIndex, ItemInfo_t itemInfo, BOOL setPosition);
	static HRESULT ExtractFindDataUsingPropertyStore(IShellFolder *shellFolder,
		PCITEMID_CHILD pidlChild, WIN32_FIND_DATA &output);
	void SetViewModeInternal(ViewMode viewMode);
	void SetFirstColumnTextToCallback();
	void SetFirstColumnTextToFilename();

	// Shell window integration
	void NotifyShellOfNavigation(PCIDLIST_ABSOLUTE pidl);
	HRESULT RegisterShellWindowIfNecessary(PCIDLIST_ABSOLUTE pidl);
	HRESULT RegisterShellWindow(PCIDLIST_ABSOLUTE pidl);

	LRESULT ListViewProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT ListViewParentProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static int CALLBACK SortStub(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

	/* Message handlers. */
	void ColumnClicked(int iClickedColumn);

	/* Listview. */
	void OnListViewLeftButtonDoubleClick(const POINT *pt);
	void OnListViewMButtonDown(const POINT *pt);
	void OnListViewMButtonUp(const POINT *pt, UINT keysDown);
	void OnRButtonDown(HWND hwnd, BOOL doubleClick, int x, int y, UINT keyFlags);
	void OnListViewGetDisplayInfo(LPARAM lParam);
	LRESULT OnListViewGetInfoTip(NMLVGETINFOTIP *getInfoTip);
	BOOL OnListViewGetEmptyMarkup(NMLVEMPTYMARKUP *emptyMarkup);
	void QueueInfoTipTask(int internalIndex, const std::wstring &existingInfoTip);
	static std::optional<InfoTipResult> GetInfoTipAsync(HWND listView, int infoTipResultId,
		int internalIndex, const BasicItemInfo_t &basicItemInfo, const Config &config,
		HINSTANCE resourceInstance, bool virtualFolder);
	void ProcessInfoTipResult(int infoTipResultId);
	void OnListViewItemInserted(const NMLISTVIEW *itemData);
	void OnListViewItemChanged(const NMLISTVIEW *changeData);
	void UpdateFileSelectionInfo(int internalIndex, BOOL selected);
	void OnListViewKeyDown(const NMLVKEYDOWN *lvKeyDown);
	std::vector<PCIDLIST_ABSOLUTE> GetSelectedItemPidls();
	void OnListViewBeginDrag(const NMLISTVIEW *info);
	void OnListViewBeginRightClickDrag(const NMLISTVIEW *info);
	HRESULT StartDrag(int draggedItem, const POINT &startPoint);
	BOOL OnListViewBeginLabelEdit(const NMLVDISPINFO *dispInfo);
	BOOL OnListViewEndLabelEdit(const NMLVDISPINFO *dispInfo);
	LRESULT OnListViewCustomDraw(NMLVCUSTOMDRAW *listViewCustomDraw);
	void OnColorRulesUpdated();
	void OnFullRowSelectUpdated(BOOL newValue);

	HRESULT GetListViewItemAttributes(int item, SFGAOF *attributes) const;

	void StartRenamingSingleFile();
	void StartRenamingMultipleFiles();

	// Listview header context menu
	void OnListViewHeaderRightClick(const POINTS &cursorPos);
	void OnListViewHeaderMenuItemSelected(int menuItemId,
		const std::unordered_map<int, ColumnType> &menuItemMappings);
	void OnShowMoreColumnsSelected();
	void OnColumnMenuItemSelected(int menuItemId,
		const std::unordered_map<int, ColumnType> &menuItemMappings);

	const ItemInfo_t &GetItemByIndex(int index) const;
	ItemInfo_t &GetItemByIndex(int index);
	int GetItemInternalIndex(int item) const;

	BasicItemInfo_t getBasicItemInfo(int internalIndex) const;

	/* Sorting. */
	void SortFolder();
	int CALLBACK Sort(int InternalIndex1, int InternalIndex2) const;

	/* Listview column support. */
	void AddFirstColumn();
	void SetUpListViewColumns();
	void DeleteAllColumns();
	void QueueColumnTask(int itemInternalIndex, ColumnType columnType);
	static ColumnResult_t GetColumnTextAsync(HWND listView, int columnResultId,
		ColumnType columnType, int internalIndex, const BasicItemInfo_t &basicItemInfo,
		const GlobalFolderSettings &globalFolderSettings);
	void InsertColumn(ColumnType columnType, int columnIndex, int width);
	void SetActiveColumnSet();
	void GetColumnInternal(ColumnType columnType, Column_t *pci) const;
	Column_t GetFirstCheckedColumn();
	void SaveColumnWidths();
	void ProcessColumnResult(int columnResultId);
	std::optional<int> GetColumnIndexByType(ColumnType columnType) const;
	std::optional<ColumnType> GetColumnTypeByIndex(int index) const;

	/* Device change support. */
	void UpdateDriveIcon(const TCHAR *szDrive);
	void RemoveDrive(const TCHAR *szDrive);

	/* Directory altered support. */
	void StartDirectoryMonitoring(PCIDLIST_ABSOLUTE pidl);
	void ProcessShellChangeNotifications(
		const std::vector<ShellChangeNotification> &shellChangeNotifications);
	void ProcessShellChangeNotification(const ShellChangeNotification &change);
	void OnItemAdded(PCIDLIST_ABSOLUTE simplePidl);
	void AddItem(PCIDLIST_ABSOLUTE pidl);
	void RemoveItem(int iItemInternal);
	void OnItemRemoved(PCIDLIST_ABSOLUTE simplePidl);
	void OnItemModified(PCIDLIST_ABSOLUTE simplePidl);
	void UpdateItem(PCIDLIST_ABSOLUTE pidl, PCIDLIST_ABSOLUTE updatedPidl = nullptr);
	void OnItemRenamed(PCIDLIST_ABSOLUTE simplePidlOld, PCIDLIST_ABSOLUTE simplePidlNew);
	void InvalidateAllColumnsForItem(int itemIndex);
	void InvalidateIconForItem(int itemIndex);
	int DetermineItemSortedPosition(LPARAM lParam) const;

	/* Filtering support. */
	void UpdateFiltering();
	void RemoveFilteredItems();
	void RemoveFilteredItem(int iItem, int iItemInternal);
	BOOL IsFilenameFiltered(const TCHAR *FileName) const;
	void UnfilterAllItems();
	void UnfilterItem(int internalIndex);
	void RestoreFilteredItem(int internalIndex);

	/* Listview group support. */
	static int CALLBACK GroupComparisonStub(int id1, int id2, void *data);
	int GroupComparison(int id1, int id2);
	int GroupNameComparison(const ListViewGroup &group1, const ListViewGroup &group2);
	int GroupRelativePositionComparison(const ListViewGroup &group1, const ListViewGroup &group2);
	const ListViewGroup GetListViewGroupById(int groupId);
	int DetermineItemGroup(int iItemInternal);
	std::optional<GroupInfo> DetermineItemNameGroup(const BasicItemInfo_t &itemInfo) const;
	std::optional<GroupInfo> DetermineItemSizeGroup(const BasicItemInfo_t &itemInfo) const;
	std::optional<GroupInfo> DetermineItemTotalSizeGroup(const BasicItemInfo_t &itemInfo) const;
	std::optional<GroupInfo> DetermineItemTypeGroupVirtual(const BasicItemInfo_t &itemInfo) const;
	std::optional<GroupInfo> DetermineItemDateGroup(const BasicItemInfo_t &itemInfo,
		GroupByDateType dateType) const;
	std::optional<GroupInfo> DetermineItemSummaryGroup(const BasicItemInfo_t &itemInfo,
		const SHCOLUMNID *pscid, const GlobalFolderSettings &globalFolderSettings) const;
	std::optional<GroupInfo> DetermineItemFreeSpaceGroup(const BasicItemInfo_t &itemInfo) const;
	std::optional<GroupInfo> DetermineItemAttributeGroup(const BasicItemInfo_t &itemInfo) const;
	std::optional<GroupInfo> DetermineItemOwnerGroup(const BasicItemInfo_t &itemInfo) const;
	std::optional<GroupInfo> DetermineItemVersionGroup(const BasicItemInfo_t &itemInfo,
		const TCHAR *szVersionType) const;
	std::optional<GroupInfo> DetermineItemCameraPropertyGroup(const BasicItemInfo_t &itemInfo,
		PROPID PropertyId) const;
	std::optional<GroupInfo> DetermineItemExtensionGroup(const BasicItemInfo_t &itemInfo) const;
	std::optional<GroupInfo> DetermineItemFileSystemGroup(const BasicItemInfo_t &itemInfo) const;
	std::optional<GroupInfo> DetermineItemNetworkStatus(const BasicItemInfo_t &itemInfo) const;

	/* Other grouping support. */
	int GetOrCreateListViewGroup(const GroupInfo &groupInfo);
	void MoveItemsIntoGroups();
	void InsertItemIntoGroup(int index, int groupId);
	void EnsureGroupExistsInListView(int groupId);
	void InsertGroupIntoListView(const ListViewGroup &listViewGroup);
	void RemoveGroupFromListView(const ListViewGroup &listViewGroup);
	void UpdateGroupHeader(const ListViewGroup &listViewGroup);
	std::wstring GenerateGroupHeader(const ListViewGroup &listViewGroup);
	void OnItemRemovedFromGroup(int groupId);
	void OnItemAddedToGroup(int groupId);
	std::optional<int> GetItemGroupId(int index);

	/* Listview icons. */
	void ProcessIconResult(int internalIndex, int iconIndex);
	std::optional<int> GetCachedIconIndex(const ItemInfo_t &itemInfo);

	/* Thumbnails view. */
	void QueueThumbnailTask(int internalIndex);
	std::optional<int> GetCachedThumbnailIndex(const ItemInfo_t &itemInfo);
	static wil::unique_hbitmap GetThumbnail(PIDLIST_ABSOLUTE pidl, WTS_FLAGS flags);
	void ProcessThumbnailResult(int thumbnailResultId);
	void SetupThumbnailsView();
	void RemoveThumbnailsView();
	int GetIconThumbnail(int iInternalIndex) const;
	int GetExtractedThumbnail(HBITMAP hThumbnailBitmap) const;
	int GetThumbnailInternal(int iType, int iInternalIndex, HBITMAP hThumbnailBitmap) const;
	void DrawIconThumbnailInternal(HDC hdcBacking, int iInternalIndex) const;
	void DrawThumbnailInternal(HDC hdcBacking, HBITMAP hThumbnailBitmap) const;

	/* Tiles view. */
	void InsertTileViewColumns();
	void SetTileViewInfo();
	void SetTileViewItemInfo(int iItem, int iItemInternal);

	void UpdateCurrentClipboardObject(wil::com_ptr_nothrow<IDataObject> clipboardDataObject);
	void OnClipboardUpdate();
	void RestoreStateOfCutItems();

	// ShellDropTargetWindow
	int GetDropTargetItem(const POINT &pt) override;
	unique_pidl_absolute GetPidlForTargetItem(int targetItem) override;
	IUnknown *GetSiteForTargetItem(PCIDLIST_ABSOLUTE targetItemPidl) override;
	bool IsTargetSourceOfDrop(int targetItem, IDataObject *dataObject) override;
	void UpdateUiForDrop(int targetItem, const POINT &pt) override;
	void ResetDropUiState() override;

	/* Drag and Drop support. */
	void RepositionLocalFiles(const POINT *ppt);
	void ScrollListViewForDrop(const POINT &pt);
	void PositionDroppedItems();

	void OnApplicationShuttingDown();

	/* Miscellaneous. */
	BOOL CompareVirtualFolders(UINT uFolderCSIDL) const;
	int LocateFileItemInternalIndex(const TCHAR *szFileName) const;
	std::optional<int> GetItemIndexForPidl(PCIDLIST_ABSOLUTE pidl) const;
	std::optional<int> GetItemInternalIndexForPidl(PCIDLIST_ABSOLUTE pidl) const;
	std::optional<int> LocateItemByInternalIndex(int internalIndex) const;
	void ApplyHeaderSortArrow();

	HWND m_hListView;
	HWND m_hOwner;

	NavigationStartedSignal m_navigationStartedSignal;
	NavigationCommittedSignal m_navigationCommittedSignal;
	NavigationCompletedSignal m_navigationCompletedSignal;
	NavigationFailedSignal m_navigationFailedSignal;
	std::unique_ptr<ShellNavigationController> m_navigationController;

	TabNavigationInterface *m_tabNavigation;
	FileActionHandler *m_fileActionHandler;

	std::vector<std::unique_ptr<WindowSubclassWrapper>> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;

	HIMAGELIST m_hListViewImageList;

	DirectoryState m_directoryState;

	/* Stores various extra information on files, such
	as display name. */
	std::unordered_map<int, ItemInfo_t> m_itemInfoMap;

	ctpl::thread_pool m_columnThreadPool;
	std::unordered_map<int, std::future<ColumnResult_t>> m_columnResults;
	int m_columnResultIDCounter;

	std::unique_ptr<IconFetcher> m_iconFetcher;
	CachedIcons *m_cachedIcons;

	IconResourceLoader *m_iconResourceLoader;

	ctpl::thread_pool m_thumbnailThreadPool;
	std::unordered_map<int, std::future<std::optional<ThumbnailResult_t>>> m_thumbnailResults;
	int m_thumbnailResultIDCounter;

	ctpl::thread_pool m_infoTipsThreadPool;
	std::unordered_map<int, std::future<std::optional<InfoTipResult>>> m_infoTipResults;
	int m_infoTipResultIDCounter;

	/* Internal state. */
	const HINSTANCE m_resourceInstance;
	HACCEL *m_acceleratorTable;
	BOOL m_bFolderVisited;
	std::optional<int> m_dirMonitorId;
	int m_iFolderIcon;
	int m_iFileIcon;
	int m_iDropped;

	/* Stores a unique index for each folder.
	This may be needed so that folders can be
	told apart when adding files from directory
	modification. */
	int m_uniqueFolderId;

	const Config *m_config;
	FolderSettings m_folderSettings;

	/* ID. */
	std::optional<int> m_ID;

	// Directory monitoring
	ShellChangeWatcher m_shellChangeWatcher;
	unique_pidl_absolute m_renamedItemOldPidl;

	/* Stores information on files that
	have been modified (i.e. created, deleted,
	renamed, etc). */
	CRITICAL_SECTION m_csDirectoryAltered;
	std::list<AlteredFile_t> m_AlteredList;

	int m_middleButtonItem;

	// Shell window integration
	static inline winrt::com_ptr<IShellWindows> m_shellWindows;
	bool m_shellWindowRegistered;
	unique_shell_window_cookie m_shellWindowCookie;

	/* Shell new. */
	unique_pidl_absolute m_queuedRenameItem;

	/* File selection. */
	std::list<std::wstring> m_FileSelectionList;

	/* Thumbnails. */
	BOOL m_bThumbnailsSetup;

	/* Column related data. */
	std::vector<Column_t> *m_pActiveColumns;
	FolderColumns m_folderColumns;
	int m_nCurrentColumns;
	int m_nActiveColumns;
	bool m_PreviousSortColumnExists;
	ColumnType m_previousSortColumn;

	wil::com_ptr_nothrow<IDataObject> m_clipboardDataObject;
	std::vector<std::wstring> m_cutFileNames;

	/* Drag and drop related data. */
	UINT m_getDragImageMessage;
	winrt::com_ptr<ServiceProvider> m_dropServiceProvider;
	std::vector<unique_pidl_absolute> m_draggedItems;
	POINT m_ptDraggedOffset;
	bool m_performingDrag;
	IDataObject *m_draggedDataObject;
	std::list<DroppedFile_t> m_droppedFileNameList;

	ListViewGroupSet m_listViewGroups;
	int m_groupIdCounter;
};
