// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UNDO_UNDO_MANAGER_H_
#define CHROME_BROWSER_UNDO_UNDO_MANAGER_H_

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "base/strings/string16.h"

class UndoOperation;

// UndoGroup ------------------------------------------------------------------

// UndoGroup represents a user action and stores all the operations that
// make that action.  Typically there is only one operation per UndoGroup.
class UndoGroup {
 public:
  UndoGroup();
  ~UndoGroup();

  void AddOperation(scoped_ptr<UndoOperation> operation);
  const std::vector<UndoOperation*>& undo_operations() {
    return operations_.get();
  }
  void Undo();

  // The resource string id describing the undo and redo action.
  int get_undo_label_id() const { return undo_label_id_; }
  void set_undo_label_id(int label_id) { undo_label_id_ = label_id; }

  int get_redo_label_id() const { return redo_label_id_; }
  void set_redo_label_id(int label_id) { redo_label_id_ = label_id; }

 private:
  ScopedVector<UndoOperation> operations_;

  // The resource string id describing the undo and redo action.
  int undo_label_id_;
  int redo_label_id_;

  DISALLOW_COPY_AND_ASSIGN(UndoGroup);
};

// UndoManager ----------------------------------------------------------------

// Maintains user actions as a group of operations that store enough info to
// undo and redo those operations.
class UndoManager {
 public:
  UndoManager();
  ~UndoManager();

  // Perform an undo or redo operation.
  void Undo();
  void Redo();

  size_t undo_count() const { return undo_actions_.size(); }
  size_t redo_count() const { return redo_actions_.size(); }

  base::string16 GetUndoLabel() const;
  base::string16 GetRedoLabel() const;

  void AddUndoOperation(scoped_ptr<UndoOperation> operation);

  // Group multiple operations into one undoable action.
  void StartGroupingActions();
  void EndGroupingActions();

  // Suspend undo tracking while processing non-user initiated changes such as
  // profile synchonization.
  void SuspendUndoTracking();
  void ResumeUndoTracking();
  bool IsUndoTrakingSuspended() const;

  // Returns all UndoOperations that are awaiting Undo or Redo. Note that
  // ownership of the UndoOperations is retained by UndoManager.
  std::vector<UndoOperation*> GetAllUndoOperations() const;

  // Remove all undo and redo operations. Note that grouping of actions and
  // suspension of undo tracking states are left unchanged.
  void RemoveAllOperations();

 private:
  void Undo(bool* performing_indicator,
            ScopedVector<UndoGroup>* active_undo_group);
  bool is_user_action() const { return !performing_undo_ && !performing_redo_; }

  // Handle the addition of |new_undo_group| to the active undo group container.
  void AddUndoGroup(UndoGroup* new_undo_group);

  // Returns the undo or redo UndoGroup container that should store the next
  // change taking into account if an undo or redo is being executed.
  ScopedVector<UndoGroup>* GetActiveUndoGroup();

  // Containers of user actions ready for an undo or redo treated as a stack.
  ScopedVector<UndoGroup> undo_actions_;
  ScopedVector<UndoGroup> redo_actions_;

  // Supports grouping operations into a single undo action.
  int group_actions_count_;

  // The container that is used when actions are grouped.
  scoped_ptr<UndoGroup> pending_grouped_action_;

  // The action that is in the process of being undone.
  UndoGroup* undo_in_progress_action_;

  // Supports the suspension of undo tracking.
  int undo_suspended_count_;

  // Set when executing Undo or Redo so that incoming changes are correctly
  // processed.
  bool performing_undo_;
  bool performing_redo_;

  DISALLOW_COPY_AND_ASSIGN(UndoManager);
};

#endif  // CHROME_BROWSER_UNDO_UNDO_MANAGER_H_
