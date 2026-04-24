//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_ESF_USD_STAGE_DATA_H
#define PXR_EXEC_ESF_USD_STAGE_DATA_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/esfUsd/api.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/hash.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/notice.h"

#ifdef TBB_PREVIEW_CONCURRENT_ORDERED_CONTAINERS
#include <tbb/concurrent_map.h>
#else
#define TBB_PREVIEW_CONCURRENT_ORDERED_CONTAINERS 1
#include <tbb/concurrent_map.h>
#undef TBB_PREVIEW_CONCURRENT_ORDERED_CONTAINERS
#endif

#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_unordered_set.h>
#include <tbb/concurrent_vector.h>

#include <memory>
#include <mutex>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class UsdAttribute;
class UsdPrim;

TF_DECLARE_WEAK_PTRS(UsdStage);

/// Class that holds data that is cached per-stage.
///
/// \note
/// It's unfortunate that clients that access outgoing and incoming connections
/// using this class have to provide a stage pointer, making the implementation
/// of this class less performant and more complicated. Ideally, EsfUsd objects
/// would have a way to get directly to the stage data for their stage, but
/// given the fact that those objects are all short-lived, it's not clear how to
/// do that, at least without growing the sizes of all Esf objects.
///
class EsfUsdStageData
{
    EsfUsdStageData(const UsdStageConstPtr &stage);

public:

    /// A concurrent set of paths, used to indicate the set of targets for which
    /// incoming connections have changed.
    ///
    using ChangedPathSet = tbb::concurrent_unordered_set<SdfPath, TfHash>;

    /// The base class for listeners defined by clients in order to be notified
    /// of scene changes.
    ///
    class ESFUSD_API_TYPE ListenerBase {
    public:
        ESFUSD_API
        virtual ~ListenerBase();

    private:
        friend EsfUsdStageData;

        /// Called to communicate scene changes to a client liseter.
        ///
        virtual void _DidObjectsChanged(
            const UsdNotice::ObjectsChanged &objectsChanged,
            const ChangedPathSet &changedTargetPaths) const = 0;
    };
    

    ESFUSD_API
    ~EsfUsdStageData();

    /// Registers \p stage as a stage for which cached data should be held,
    /// returning a strong reference the client must hold until the cached data
    /// is no longer needed.
    ///
    /// \p listener will be notified of scene changes.
    ///
    ESFUSD_API
    static std::shared_ptr<EsfUsdStageData> RegisterStage(
        const UsdStageConstPtr &stage,
        const ListenerBase *listener);

    /// Notifies that \p listener no longer needs to be informed of changes.
    ///
    ESFUSD_API
    void Unregister(
        const ListenerBase *listener);

    /// Get the cached stage data for \p stage.
    ///
    /// \note
    /// The client calling this method must first have registered \p stage by
    /// calling RegisterStage and must still be holding the strong reference to
    /// the stage data while calling this method and using the returned
    /// reference.
    ///
    ESFUSD_API
    static EsfUsdStageData &GetStageData(
        const UsdStageConstPtr &stage);

    /// Returns the paths of all objects that are targets of connections owned
    /// by the attribute at \p attrPath.
    ///
    ESFUSD_API
    static const SdfPathVector &GetOutgoingConnections(
        const UsdStageConstPtr &stage,
        const SdfPath &attrPath);

    /// Returns the paths of all attributes that own connections that target
    /// the object at \p targetPath.
    ///
    ESFUSD_API
    static SdfPathVector GetIncomingConnections(
        const UsdStageConstPtr &stage,
        const SdfPath &targetPath);

private:
    const UsdStageConstPtr &_GetStage() const { return _stage; }

    const SdfPathVector &_GetOutgoingConnections(
        const SdfPath &targetPath);

    SdfPathVector _GetIncomingConnections(
        const SdfPath &targetPath);

    void _PopulateConnectionTables();

    // Define a type and a corresponding transparent comparator so that we can
    // use lower_bound to find the end of a map range with keys that are paths
    // with a given prefix.

    struct _PathPrefix
    {
        SdfPath prefix;
    };

    struct _PathRangeLessThan
    {
        using is_transparent = void;

        bool operator()(const SdfPath &lhs, const SdfPath &rhs) const {
            return lhs < rhs;
        }

        bool operator()(const SdfPath &lhs, const _PathPrefix& rhs) const {
            return lhs < rhs.prefix || lhs.HasPrefix(rhs.prefix);
        }
    };

    // A concurrent map from owning attribute paths to target object paths.
    using _OutgoingPathTable =
        tbb::concurrent_map<SdfPath, SdfPathVector, _PathRangeLessThan>;

    // A concurent map from target object paths to owning attribute paths.
    using _IncomingPathTable =
        tbb::concurrent_unordered_map<
            SdfPath, tbb::concurrent_vector<SdfPath>, TfHash>;

    // Gets the new (i.e., current) connections for the given attribute and
    // computes the added and removed target paths, relative to what was
    // previously stored in the outgoing connection paths table.
    //
    void _GetChangedConnectionTargets(
        const SdfPath &attrPath,
        const UsdAttribute &attribute,
        SdfPathVector *newConnections,
        SdfPathVector *addedTargetPaths,
        SdfPathVector *removedTargetPaths) const;

    // Updates attribute connection caches for connections owned by the
    // attribute at \p attrPath.
    //
    // Populates \p incomingConnectionsChanged with the paths of objects whose
    // incoming connection paths have changed.
    //
    void _UpdateForChangedAttributeConnections(
        const SdfPath &attrPath,
        ChangedPathSet *incomingConnectionsChanged);

    // Updates attribute connection caches for connections owned by attribute
    // at or under \p resyncedPath.
    //
    void _UpdateForResync(
        const SdfPath &resyncedPath,
        ChangedPathSet *incomingConnectionsChanged);

    // Updates outgoing and incoming connection tables for any changes to the
    // given prim.
    //
    // New incoming connections are added immediately; new outgoing connections
    // and removed incoming connections are queued up for deferred processing,
    // to allow this method to be called in parallel.
    //
    void _UpdateForChangedPrim(
        const UsdPrim &prim,
        _IncomingPathTable *incomingToRemove,
        ChangedPathSet *incomingConnectionsChanged);

    // Find entries in the outgoing connections map for attributes on the
    // resynced prim and its descendants that no longer exist in the scene,
    // populating \p ownerAttrsToRemove and \p incomingToRemove.
    //
    void _UpdateForRemovedAttributes(
        const SdfPath &resyncedPath,
        _IncomingPathTable *incomingToRemove,
        tbb::concurrent_vector<SdfPath> *ownerAttrsToRemove) const;

    // Removes the entries indicated by \p incomingToRemove from _incoming.
    void _RemoveIncomingTableEntries(
        const _IncomingPathTable &incomingToRemove);

    // Notifies all listeners of changes.
    void _Notify(
        const UsdNotice::ObjectsChanged &objectsChanged,
        const ChangedPathSet &changedTargetPaths) const;

private:
    const UsdStageConstPtr _stage;

    // Used to listen for change notification from the stage.
    class _NoticeListener;
    const std::unique_ptr<_NoticeListener> _noticeListener;

    // Registered listeners, used to communicate change notfication to clients.
    std::vector<const ListenerBase *> _listeners;
    mutable std::mutex _listenersMutex;

    _OutgoingPathTable _outgoing;

    _IncomingPathTable _incoming;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
