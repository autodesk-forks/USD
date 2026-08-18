#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

import unittest
from pxr import Tf

class TestTfDiagnosticTrap(unittest.TestCase):
    """
    Test TfDiagnosticTrap, catching Tf.Status and Tf.Warn.
    """
    def test_BasicCapture(self):
        # Verify that warnings and statuses are captured and inspectable during
        # the trap's lifetime.
        with Tf.DiagnosticTrap() as trap:
            self.assertFalse(trap.HasWarnings())
            self.assertFalse(trap.HasStatuses())

            Tf.Warn("test warning")
            self.assertTrue(trap.HasWarnings())
            self.assertEqual(len(trap.GetWarnings()), 1)
            self.assertEqual(trap.GetWarnings()[0].commentary, "test warning")
            self.assertFalse(trap.HasStatuses())

            Tf.Status("test status")
            self.assertTrue(trap.HasStatuses())
            self.assertEqual(len(trap.GetStatuses()), 1)
            self.assertEqual(trap.GetStatuses()[0].commentary, "test status")

            trap.Clear()
            self.assertFalse(trap.HasWarnings())
            self.assertFalse(trap.HasStatuses())

    def test_SelectiveClearing(self):
        # Verify that ClearWarnings/ClearStatuses only clear their respective
        # collections.
        with Tf.DiagnosticTrap() as trap:
            Tf.Warn("warning")
            Tf.Status("status")

            trap.ClearWarnings()
            self.assertFalse(trap.HasWarnings())
            self.assertTrue(trap.HasStatuses())

            trap.ClearStatuses()
            self.assertFalse(trap.HasStatuses())

    def test_Nesting(self):
        # Verify that the innermost trap gates diagnostics. The outer trap sees
        # nothing while the inner trap is active.
        with Tf.DiagnosticTrap() as outer:
            with Tf.DiagnosticTrap() as inner:
                Tf.Warn("inner warning")
                Tf.Status("inner status")
                self.assertTrue(inner.HasWarnings())
                self.assertTrue(inner.HasStatuses())
                self.assertFalse(outer.HasWarnings())
                self.assertFalse(outer.HasStatuses())
                inner.Clear()
            # inner exited with nothing uncleared, outer sees nothing
            self.assertFalse(outer.HasWarnings())
            self.assertFalse(outer.HasStatuses())
            outer.Clear()

    def test_RepostOnExit(self):
        # Verify that uncleared diagnostics are re-posted to the enclosing trap
        # on exit, and cleared ones are not.
        with Tf.DiagnosticTrap() as outer:
            with Tf.DiagnosticTrap() as inner:
                Tf.Warn("should be cleared")
                inner.ClearWarnings()
                Tf.Warn("should be reposted")
            # inner exited, re-posted only "should be reposted"
            self.assertTrue(outer.HasWarnings())
            self.assertEqual(len(outer.GetWarnings()), 1)
            self.assertEqual(
                outer.GetWarnings()[0].commentary, "should be reposted")
            outer.Clear()

    def test_Ordering(self):
        # Verify interleaved ordering using both ForEach and the iterator
        # protocol, asserting both produce the same result.
        with Tf.DiagnosticTrap() as outer:
            with Tf.DiagnosticTrap() as inner:
                Tf.Warn("first")
                Tf.Status("second")
                Tf.Warn("third")

            expected = [
                (Tf.Warning, "first"),
                (Tf.StatusObject, "second"),
                (Tf.Warning, "third"),
            ]

            # ForEach()
            order = []
            outer.ForEach(lambda d: order.append((type(d), d.commentary)))
            self.assertEqual(order, expected)

            # Iterate.
            self.assertEqual([(type(d), d.commentary) for d in outer], expected)

            outer.Clear()

    def test_EraseMatching(self):
        # Verify type-specific erase via isinstance.
        with Tf.DiagnosticTrap() as trap:
            Tf.Warn("keep this warning")
            Tf.Warn("erase this warning")
            Tf.Status("keep this status")

            trap.EraseMatching(lambda d:
                               isinstance(d, Tf.Warning) and
                               "erase" in d.commentary)

            self.assertTrue(trap.HasWarnings())
            self.assertEqual(len(trap.GetWarnings()), 1)
            self.assertEqual(trap.GetWarnings()[0].commentary,
                             "keep this warning")
            self.assertTrue(trap.HasStatuses())
            trap.Clear()

    def test_EraseMatchingPreservesOrder(self):
        # Verify that interleaved order is preserved after erase.
        with Tf.DiagnosticTrap() as outer:
            with Tf.DiagnosticTrap() as inner:
                Tf.Warn("first")
                Tf.Status("second")
                Tf.Warn("third")
                Tf.Status("fourth")

                inner.EraseMatching(lambda d:
                                    isinstance(d, Tf.Warning) and
                                    d.commentary == "first")

            order = [(type(d), d.commentary) for d in outer]
            self.assertEqual(order, [
                (Tf.StatusObject, "second"),
                (Tf.Warning, "third"),
                (Tf.StatusObject, "fourth")
            ])
            outer.Clear()

    def test_EraseMatchingDuringForEach(self):
        # Verify that EraseMatching is safe to call during ForEach iteration.
        with Tf.DiagnosticTrap() as trap:
            Tf.Warn("keep")
            Tf.Warn("erase")
            Tf.Warn("keep")

            def _visitor(d):
                if isinstance(d, Tf.Warning) and d.commentary == "erase":
                    trap.EraseMatching(lambda d:
                                       isinstance(d, Tf.Warning) and
                                       d.commentary == "erase")

            trap.ForEach(_visitor)

            self.assertEqual(len(trap.GetWarnings()), 2)
            self.assertEqual(trap.GetWarnings()[0].commentary, "keep")
            self.assertEqual(trap.GetWarnings()[1].commentary, "keep")
            trap.Clear()

    def test_Dismiss(self):
        # Verify that Dismiss() re-posts uncleared diagnostics and deactivates
        # the trap, leaving the __exit__ as a no-op.
        with Tf.DiagnosticTrap() as outer:
            with Tf.DiagnosticTrap() as inner:
                Tf.Warn("warning")
                inner.Dismiss()
                # Dismiss cleared the collections
                self.assertFalse(inner.HasWarnings())
            # __exit__ was a no-op, outer still received the re-post
            self.assertTrue(outer.HasWarnings())
            self.assertEqual(
                outer.GetWarnings()[0].commentary, "warning")
            outer.Clear()

    def test_IsClean(self):
        # Verify IsClean reflects whether any diagnostics are captured.
        with Tf.DiagnosticTrap() as trap:
            self.assertTrue(trap.IsClean())
            Tf.Warn("warning")
            self.assertFalse(trap.IsClean())
            trap.Clear()
            self.assertTrue(trap.IsClean())

    def test_HasAnyMatching(self):
        # Type-specific predicate.
        with Tf.DiagnosticTrap() as trap:
            Tf.Warn("deprecated feature")
            Tf.Warn("unrelated warning")
            Tf.Status("a status")

            self.assertTrue(trap.HasAnyMatching(
                lambda d: isinstance(d, Tf.Warning) and
                "deprecated" in d.commentary))
            self.assertFalse(trap.HasAnyMatching(
                lambda d: isinstance(d, Tf.Warning) and
                "nonexistent" in d.commentary))
            # Predicate returns False for non-warning types.
            self.assertFalse(trap.HasAnyMatching(
                lambda d: isinstance(d, Tf.Error)))
            trap.Clear()

    def test_HasAnyMatchingBase(self):
        # TfDiagnosticBase predicate matches across all types.
        with Tf.DiagnosticTrap() as trap:
            Tf.Warn("warning")
            Tf.Status("status")

            self.assertTrue(trap.HasAnyMatching(
                lambda d: d.commentary == "status"))
            self.assertFalse(trap.HasAnyMatching(
                lambda d: d.commentary == "nonexistent"))
            trap.Clear()

    def test_HasAnyMatchingEmpty(self):
        # Empty trap always returns False.
        with Tf.DiagnosticTrap() as trap:
            self.assertFalse(trap.HasAnyMatching(lambda d: True))

    def test_HasAllMatching(self):
        # All match, then some don't.
        with Tf.DiagnosticTrap() as trap:
            Tf.Warn("deprecated a")
            Tf.Warn("deprecated b")

            self.assertTrue(trap.HasAllMatching(
                lambda d: isinstance(d, Tf.Warning) and
                "deprecated" in d.commentary))
            
            Tf.Warn("unrelated")
            self.assertFalse(trap.HasAllMatching(
                lambda d: isinstance(d, Tf.Warning) and
                "deprecated" in d.commentary))
            trap.Clear()

    def test_CountMatching(self):
        # Type-specific and base predicate counting.
        with Tf.DiagnosticTrap() as trap:
            Tf.Warn("deprecated a")
            Tf.Warn("deprecated b")
            Tf.Warn("unrelated")
            Tf.Status("deprecated status")

            # Type-specific -- only counts warnings.
            self.assertEqual(trap.CountMatching(
                lambda d: isinstance(d, Tf.Warning) and
                "deprecated" in d.commentary), 2)
            
            # Base predicate -- counts across all types.
            self.assertEqual(trap.CountMatching(
                lambda d: "deprecated" in d.commentary), 3)
            
            # No matching type -- counts nothing.
            self.assertEqual(trap.CountMatching(
                lambda d: isinstance(d, Tf.Error)), 0)

            trap.Clear()    

if __name__ == "__main__":
    unittest.main()
