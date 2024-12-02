#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import sys, unittest
from pxr import Sdf, Usd
from testUsdNamespaceEditorDependentEditsBase \
      import TestUsdNamespaceEditorDependentEditsBase

class TestUsdNamespaceEditorDependentEditsBasicReferencesAndPayloads(
    TestUsdNamespaceEditorDependentEditsBase):
    '''Tests downstream dependency namespace edits across references and 
    payloads.'''

    def test_BasicDependentReferences(self):
        """Test downstream dependency namespace edits across basic single
        references, both internal and in separate layers """

        # Setup:
        # Layer 1 has simple hierarchy of /Ref, Child, and GrandChild that is
        # referenced and will be namespace edited. It also has three prims
        # that internally reference /Ref, /Ref/Child, and /Ref/Child/GrandChild
        # respectively. These internally referencing prims will be affected by
        # downstream dependency edits.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1ImportString = '''#usda 1.0
            def "Ref" {
                int refAttr

                def "Child" {
                    int childAttr

                    def "GrandChild" {
                        int grandChildAttr
                    }
                }
            }

            def "InternalRef1" (
                references = </Ref>
            ) {
                over "Child" {
                    int overChildAttr
                    over "GrandChild" {
                        int overGrandChildAttr
                    }
                }
                def "LocalChild" {}
                int localAttr
            }

            def "InternalRef2" (
                references = </Ref/Child>
            ) {
                over "GrandChild" {
                    int overGrandChildAttr
                }
                def "LocalChild" {}
                int localAttr
            }

            def "InternalRef3" (
                references = </Ref/Child/GrandChild>
            ) {
                def "LocalChild" {}
                int localAttr
            }
        '''
        layer1.ImportFromString(layer1ImportString)

        # Layer 2 has three prims that reference /Ref, /Ref/Child, and
        # /Ref/Child/GrandChild from layer 1 respectively. These prims will be
        # affected by downstream dependency edits.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2ImportString = '''#usda 1.0
            def "Prim1" (
                references = @''' + layer1.identifier + '''@</Ref>
            ) {
                over "Child" {
                    int overChildAttr
                    over "GrandChild" {
                        int overGrandChildAttr
                    }
                }
                def "LocalChild" {}
                int localAttr
            }

            def "Prim2" (
                references = @''' + layer1.identifier + '''@</Ref/Child>
            ) {
                over "GrandChild" {
                    int overGrandChildAttr
                }
                def "LocalChild" {}
                int localAttr
            }

            def "Prim3" (
                references = @''' + layer1.identifier + '''@</Ref/Child/GrandChild>
            ) {
                def "LocalChild" {}
                int localAttr
            }
        '''
        layer2.ImportFromString(layer2ImportString)

        # Open both layers as stages.
        stage1 = Usd.Stage.Open(layer1, Usd.Stage.LoadAll)
        stage2 = Usd.Stage.Open(layer2, Usd.Stage.LoadAll)

        # Create an editor for stage 1 with stage 2 as an addtional dependent
        # stage.
        editor = Usd.NamespaceEditor(stage1)
        editor.AddDependentStage(stage2)

        # Verify the initial composition fields for both layers.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {
            '/InternalRef1' : {
                'references' : (Sdf.Reference(primPath='/Ref'),)
            },
            '/InternalRef2' : {
                'references' : (Sdf.Reference(primPath='/Ref/Child'),)
            },
            '/InternalRef3' : {
                'references' : (Sdf.Reference(primPath='/Ref/Child/GrandChild'),)
            },
        })

        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'references' : (Sdf.Reference(layer1.identifier, '/Ref'),)
            },
            '/Prim2' : {
                'references' : (Sdf.Reference(layer1.identifier, '/Ref/Child'),)
            },
            '/Prim3' : {
                'references' : (Sdf.Reference(layer1.identifier,
                                              '/Ref/Child/GrandChild'),)
            },
        })

        # Both the internal reference prims in stage 1 and the "regular"
        # reference prims in stage 2 will have same contents since they
        # reference the same prims and define identical local specs. So,
        # we create expected contents dictionaries that we can share for
        # verification.
        refToRefContents = {
            '.' : ['refAttr', 'localAttr'],
            'Child' : {
                '.' : ['childAttr', 'overChildAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr', 'overGrandChildAttr'],
                },
            },
            'LocalChild' : {}
        }

        refToChildContents =  {
            '.' : ['childAttr', 'localAttr'],
            'GrandChild' : {
                '.' : ['grandChildAttr', 'overGrandChildAttr'],
            },
            'LocalChild' : {},
        }

        refToGrandChildContents = {
            '.' : ['grandChildAttr', 'localAttr'],
            'LocalChild' : {},
        }

        # Verify the expected contents of stage 1
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
                'Child' : {
                    '.' : ['childAttr'],
                    'GrandChild' : {
                        '.' : ['grandChildAttr'],
                    }
                }
            },
            'InternalRef1' : refToRefContents,
            'InternalRef2' : refToChildContents,
            'InternalRef3' : refToGrandChildContents
        })

        # Verify the expected contents of stage 2.
        self._VerifyStageContents(stage2, {
            'Prim1' : refToRefContents,
            'Prim2' : refToChildContents,
            'Prim3' : refToGrandChildContents
        })

        # Edit: Rename /Ref/Child to /Ref/RenamedChild
        with self.ApplyEdits(editor, "Move /Ref/Child -> /Ref/RenamedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/Child', '/Ref/RenamedChild'))

        # Verify the updated composition fields in layer1. The internal
        # reference fields to /Ref/Child and its descendant
        # /Ref/Child/GrandChild have been updated to use the renamed paths.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {
            '/InternalRef1' : {
                'references' : (Sdf.Reference(primPath='/Ref'),)
            },
            '/InternalRef2' : {
                'references' : (Sdf.Reference(primPath='/Ref/RenamedChild'),)
            },
            '/InternalRef3' : {
                'references' : (Sdf.Reference(
                                    primPath='/Ref/RenamedChild/GrandChild'),)
            },
        })

        # Verify the udpated composition fields in layer2. The reference fields
        # to /Ref/Child and its descendant /Ref/Child/GrandChild have been
        # similarly updated to use the renamed paths.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'references' : (Sdf.Reference(layer1.identifier, '/Ref'),)
            },
            '/Prim2' : {
                'references' : (Sdf.Reference(layer1.identifier,
                                              '/Ref/RenamedChild'),)
            },
            '/Prim3' : {
                'references' : (Sdf.Reference(layer1.identifier,
                                              '/Ref/RenamedChild/GrandChild'),)
            },
        })

        # Verify the updated stage contents for both stages.
        #
        # For the both prims that directly reference /Ref, the child prim is
        # renamed to RenamedChild. Note that this means the local override specs
        # for the referencing prims have been renamed to RenamedChild too which
        # is why the composed RenamedChild prim still has both childAttr and
        # overChildAttr and the composed GrandChild remains unchanged
        #
        # The contents of the other two prims in each stage remain unchanged
        # as the update to the reference fields allow those prims to remain the
        # same.
        refToRefContents = {
            '.' : ['refAttr', 'localAttr'],
            'RenamedChild' : {
                '.' : ['childAttr', 'overChildAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr', 'overGrandChildAttr'],
                },
            },
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
                'RenamedChild' : {
                    '.' : ['childAttr'],
                    'GrandChild' : {
                        '.' : ['grandChildAttr'],
                    }
                }
            },
            'InternalRef1' : refToRefContents,
            'InternalRef2' : refToChildContents,
            'InternalRef3' : refToGrandChildContents
        })

        self._VerifyStageContents(stage2, {
            'Prim1' : refToRefContents,
            'Prim2' : refToChildContents,
            'Prim3' : refToGrandChildContents
        })

        # Edit: Reparent and rename /Ref/RenamedChild to /MovedChild
        with self.ApplyEdits(editor, "Move /Ref/RenamedChild -> /MovedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/RenamedChild', '/MovedChild'))

        # Verify the udpated composition fields in layer1. The internal
        # reference fields to /Ref/RenamedChild and its descendant
        # /Ref/RenamedChild/GrandChild have been updated to use the moved paths.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {
            '/InternalRef1' : {
                'references' : (Sdf.Reference(primPath='/Ref'),)
            },
            '/InternalRef2' : {
                'references' : (Sdf.Reference(primPath='/MovedChild'),)
            },
            '/InternalRef3' : {
                'references' : (Sdf.Reference(primPath='/MovedChild/GrandChild'),)
            },
        })

        # Verify the udpated composition fields in layer2. The reference fields
        # to /Ref/RenamedChild and its descendant /Ref/RenamedChild/GrandChild
        # have been similarly updated to use the moved paths.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'references' : (Sdf.Reference(layer1.identifier, '/Ref'),)
            },
            '/Prim2' : {
                'references' : (Sdf.Reference(layer1.identifier, '/MovedChild'),)
            },
            '/Prim3' : {
                'references' : (Sdf.Reference(layer1.identifier,
                                              '/MovedChild/GrandChild'),)
            },
        })

        # Verify the updated stage contents for both stages.
        #
        # For both prims that directly reference /Ref, since RenamedChild has
        # been moved out of /Ref, it is no longer a composed child of the
        # referencing prims. Note that the over to RenamedChild (which brought 
        # in the attributes overChildAttr and GrandChild.overChildAttr) has been
        # deleted to prevent the reintroduction of a partially specced
        # RenamedChild.
        #
        # The contents of the other two prims in each stage remain unchanged
        # as the update to the reference fields allow those prims to remain the
        # same.
        refToRefContents =  {
            '.' : ['refAttr', 'localAttr'],
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
            },
            'MovedChild' : {
                '.' : ['childAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr'],
                }
            },
            'InternalRef1' : refToRefContents,
            'InternalRef2' : refToChildContents,
            'InternalRef3' : refToGrandChildContents
        })

        self._VerifyStageContents(stage2, {
            'Prim1' : refToRefContents,
            'Prim2' : refToChildContents,
            'Prim3' : refToGrandChildContents
        })

        # Edit: Reparent and rename /MovedChild back to its original path
        # /Ref/Child
        with self.ApplyEdits(editor, "Move /MovedChild -> /Ref/Child"):
            self.assertTrue(editor.MovePrimAtPath('/MovedChild', '/Ref/Child'))

        # Verify the udpated composition fields in layer1 and layer2. All the
        # reference fields have been updated to their original paths.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {
            '/InternalRef1' : {
                'references' : (Sdf.Reference(primPath='/Ref'),)
            },
            '/InternalRef2' : {
                'references' : (Sdf.Reference(primPath='/Ref/Child'),)
            },
            '/InternalRef3' : {
                'references' : (Sdf.Reference(primPath='/Ref/Child/GrandChild'),)
            },
        })

        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'references' : (Sdf.Reference(layer1.identifier, '/Ref'),)
            },
            '/Prim2' : {
                'references' : (Sdf.Reference(layer1.identifier, '/Ref/Child'),)
            },
            '/Prim3' : {
                'references' : (Sdf.Reference(layer1.identifier,
                                              '/Ref/Child/GrandChild'),)
            },
        })

        # Verify the updated stage contents for both stages.
        #
        # For both prims that directly reference /Ref, they are returned to
        # their original contents with one notable exception: the overs on
        # Child that introduced overChildAttr and GrandChild.overGrandChildAttr
        # are NOT restored from being deleted so these attributes are not
        # present like they were in the initial state of the stages.
        #
        # The contents of the other two prims in each stage remain unchanged
        # (which matches their initial state) as the update to the reference
        # fields allow those prims to remain the same.
        refToRefContents = {
            '.' : ['refAttr', 'localAttr'],
            'Child' : {
                '.' : ['childAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr']
                }
            },
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
                'Child' : {
                    '.' : ['childAttr'],
                    'GrandChild' : {
                        '.' : ['grandChildAttr'],
                    }
                }
            },
            'InternalRef1' : refToRefContents,
            'InternalRef2' : refToChildContents,
            'InternalRef3' : refToGrandChildContents
        })

        self._VerifyStageContents(stage2, {
            'Prim1' : refToRefContents,
            'Prim2' : refToChildContents,
            'Prim3' : refToGrandChildContents
        })

        # Reset both layers before the next operation to return the overs on the
        # prims that reference /Ref
        layer1.ImportFromString(layer1ImportString)
        layer2.ImportFromString(layer2ImportString)

        # Verify the stage contents which again have the initial overs.
        refToRefContents = {
            '.' : ['refAttr', 'localAttr'],
            'Child' : {
                '.' : ['childAttr', 'overChildAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr', 'overGrandChildAttr']
                }
            },
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
                'Child' : {
                    '.' : ['childAttr'],
                    'GrandChild' : {
                        '.' : ['grandChildAttr'],
                    }
                }
            },
            'InternalRef1' : refToRefContents,
            'InternalRef2' : refToChildContents,
            'InternalRef3' : refToGrandChildContents
        })

        self._VerifyStageContents(stage2, {
            'Prim1' : refToRefContents,
            'Prim2' : refToChildContents,
            'Prim3' : refToGrandChildContents
        })

        # Edit: Delete the prim at /Ref/Child
        with self.ApplyEdits(editor, "Delete /Ref/Child"):
            self.assertTrue(editor.DeletePrimAtPath('/Ref/Child'))

        # Verify the udpated composition fields in layer1 and layer2. All the
        # reference fields that reference /Ref/Child or its descendants have
        # had those references removed. Note the reference fields remain but the
        # values are empty.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {
            '/InternalRef1' : {
                'references' : (Sdf.Reference(primPath='/Ref'),)
            },
            '/InternalRef2' : {
                'references' : ()
            },
            '/InternalRef3' : {
                'references' : ()
            },
        })

        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'references' : (Sdf.Reference(layer1.identifier, '/Ref'),)
            },
            '/Prim2' : {
                'references' : ()
            },
            '/Prim3' : {
                'references' : ()
            },
        })

        # Verify the updated stage contents for both stages.
        #
        # For both prims that directly reference /Ref, since Child has been 
        # deleted as a child of from /Ref, it is no longer a composed child of
        # these referencing prims. Note that the overs to Child (which brought
        # in the attributes overChildAttr and GrandChild.overChildAttr) have
        # been deleted to prevent the reintroduction of a partially specced
        # Child.
        #
        # Unlike all the prior edits in this test case, the contents of the
        # other two prims in each stage DO change because the references have
        # been deleted and can no longer compose contents from across them.
        # Addtionally any child specs in the referencing prims that would've
        # comosed over the now deleted referenced prim's children are also
        # deleted to "truly delete" all composed prims from the deleted
        # reference.
        #
        # XXX: There is an open question as to whether this is the correct
        # behavior or if we should go a step further and delete the specs that
        # introduced the now deleted reference prims (instead of just removing
        # the reference but not the referencing prim itself like we do now).
        # This expanded approach would result in the InternalRef2, InternalRef3,
        # Prim2, and Prim3 prims being completely deleted from their stages.
        refToRefContents = {
            '.' : ['refAttr', 'localAttr'],
            'LocalChild' : {},
        }

        refToChildContents =  {
            '.' : ['localAttr'],
            'LocalChild' : {},
        }

        refToGrandChildContents = {
            '.' : ['localAttr'],
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
            },
            'InternalRef1' : refToRefContents,
            'InternalRef2' : refToChildContents,
            'InternalRef3' : refToGrandChildContents
        })

        self._VerifyStageContents(stage2, {
            'Prim1' : refToRefContents,
            'Prim2' : refToChildContents,
            'Prim3' : refToGrandChildContents
        })

    def test_BasicDependentPayloads(self):
        """Test downstream dependency namespace edits across basic single
        payloads."""

        # Setup:
        # Layer 1 has simple hierarchy of /Ref, Child, GrandChild that is
        # referenced and will be namespace edited.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1.ImportFromString('''#usda 1.0
            def "Ref" {
                int refAttr

                def "Child" {
                    int childAttr

                    def "GrandChild" {
                        int grandChildAttr
                    }
                }
            }
        ''')

        # Layer 2 has three prims that payload /Ref, /Ref/Child, and
        # /Ref/Child/GrandChild from layer 1 respectively. These will be
        # affected by downstream dependency edits.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2.ImportFromString('''#usda 1.0
            def "Prim1" (
                payload = @''' + layer1.identifier + '''@</Ref>
            ) {
                over "Child" {
                    int overChildAttr
                    over "GrandChild" {
                        int overGrandChildAttr
                    }
                }
                def "LocalChild" {}
                int localAttr
            }

            def "Prim2" (
                payload = @''' + layer1.identifier + '''@</Ref/Child>
            ) {
                int overChildAttr
                over "GrandChild" {
                    int overGrandChildAttr
                }
                def "LocalChild" {}
                int localAttr
            }

            def "Prim3" (
                payload = @''' + layer1.identifier + '''@</Ref/Child/GrandChild>
            ) {
                def "LocalChild" {}
                int localAttr
            }
        ''')

        # Open both layers as stages.
        stage1 = Usd.Stage.Open(layer1, Usd.Stage.LoadAll)
        stage2 = Usd.Stage.Open(layer2, Usd.Stage.LoadAll)

        # Create an editor for stage 1 with stage 2 as an addtional dependent
        # stage.
        editor = Usd.NamespaceEditor(stage1)
        editor.AddDependentStage(stage2)

        # Verify the initial composition fields for both layer 2 (layer 1 has
        # no composition fields).
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'payload' : (Sdf.Payload(layer1.identifier, '/Ref'),)
            },
            '/Prim2' : {
                'payload' : (Sdf.Payload(layer1.identifier, '/Ref/Child'),)
            },
            '/Prim3' : {
                'payload' : (Sdf.Payload(layer1.identifier,
                                         '/Ref/Child/GrandChild'),)
            },
        })

        # Verify the initial simple hierarchy in stage 1.
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
                'Child' : {
                    '.' : ['childAttr'],
                    'GrandChild' : {
                        '.' : ['grandChildAttr']
                    }
                }
            }
        })

        # Verify the initial composed prim contents for each prim in stage 2.
        # Note that all prims are loaded at the start so all payload contents
        # are composed with local prim specs.
        prim1Contents = {
            '.' : ['refAttr', 'localAttr'],
            'Child' : {
                '.' : ['childAttr', 'overChildAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr', 'overGrandChildAttr']
                },
            },
            'LocalChild' : {},
        }

        prim2Contents =  {
            '.' : ['childAttr', 'overChildAttr', 'localAttr'],
            'GrandChild' : {
                '.' : ['grandChildAttr', 'overGrandChildAttr']
            },
            'LocalChild' : {},
        }

        prim3Contents = {
            '.' : ['grandChildAttr', 'localAttr'],
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
            'Prim3' : prim3Contents
        })

        # Edit: Rename /Ref/Child to /Ref/RenamedChild
        with self.ApplyEdits(editor, "Move /Ref/Child -> /Ref/RenamedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/Child', '/Ref/RenamedChild'))

        # Verify the udpated composition fields in layer2. The payload fields to
        # /Ref/Child and its descendant /Ref/Child/GrandChild have been updated
        # to use the renamed paths because all prims are currently loaded.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'payload' : (Sdf.Payload(layer1.identifier, '/Ref'),)
            },
            '/Prim2' : {
                'payload' : (Sdf.Payload(layer1.identifier, '/Ref/RenamedChild'),)
            },
            '/Prim3' : {
                'payload' : (Sdf.Payload(layer1.identifier,
                                         '/Ref/RenamedChild/GrandChild'),)
            },
        })

        # Verify the expected simple rename in stage 1.
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
                'RenamedChild' : {
                    '.' : ['childAttr'],
                    'GrandChild' : {
                        '.' : ['grandChildAttr']
                    }
                }
            },
        })

        # Verify the updated contents for stage 2.
        #
        # For /Prim1 which directly references /Ref, the child prim is renamed
        # to RenamedChild. And just like with references, this means the local
        # override specs for the prims introducing the payloads have been 
        # renamed to RenamedChild.
        #
        # The contents of the other two prims in each stage remain unchanged
        # as the update to the payload fields allow those prims to remain the
        # same.
        prim1Contents = {
            '.' : ['refAttr', 'localAttr'],
            'RenamedChild' : {
                '.' : ['childAttr', 'overChildAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr', 'overGrandChildAttr']
                },
            },
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
            'Prim3' : prim3Contents
        })

        # Now unload /Prim1 and /Prim3 so that we can demonstrate the one way
        # that payload dependencies deviate from references.
        stage2.Unload('/Prim1')
        stage2.Unload('/Prim3')

        # Verify the contents of stage 2 with the prims unloaded.
        #
        # /Prim1 and /Prim3 are only composed from the their local opinions
        # without the opinions from their payloads in layer 1
        prim1Contents = {
            '.' : ['localAttr'],
            'RenamedChild' : {
                '.' : ['overChildAttr'],
                'GrandChild' : {
                    '.' : ['overGrandChildAttr'],
                },
            },
            'LocalChild' : {},
        }

        prim3Contents = {
            '.' : ['localAttr'],
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
            'Prim3' : prim3Contents
        })

        # Edit: Rename /Ref/RenamedChild back to /Ref/Child
        with self.ApplyEdits(editor, "Move /Ref/RenamedChild -> /Ref/Child"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/RenamedChild', '/Ref/Child'))

        # Verify the udpated composition fields in layer2. ONLY the payload for
        # the still loaded /Prim2 is updated. The payload for /Prim1 would've
        # been unaffected regardless of the load state, but /Prim3's payload
        # can't be updated solely because it is unloaded.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'payload' : (Sdf.Payload(layer1.identifier, '/Ref'),)
            },
            '/Prim2' : {
                'payload' : (Sdf.Payload(layer1.identifier, '/Ref/Child'),)
            },
            '/Prim3' : {
                'payload' : (Sdf.Payload(layer1.identifier,
                                         '/Ref/RenamedChild/GrandChild'),)
            },
        })

        # Verify the contents for stage 2
        #
        # NONE of the prim contents have changed but for a different reasons.
        # /Prim1 and /Prim3 have not changed because the payloads are unloaded
        # and there is no dependency on the payload path for which to propagate
        # dependent edits.
        # /Prim2 is unchanged because the loaded payload's path has been updated
        # to the new path.
        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
            'Prim3' : prim3Contents
        })

        # Now load /Prim1 and /Prim3 again. Loading /Prim3 will emit warnings
        # because of the payload points to path that no longer exists in layer1.
        stage2.Load('/Prim1')
        print("\n=== EXPECT WARNINGS ===", file=sys.stderr)
        stage2.Load('/Prim3')
        print("\n=== END EXPECTED WARNINGS ===", file=sys.stderr)

        # Verify the post-edit contents for stage 2 now with all prims loaded.
        #
        # The contents of /Prim1 and /Prim3 with payloads loaded show how we
        # couldn't correctly update the dependent paths for the unloaded 
        # payloads when doing the prior rename back.
        #
        # The now loaded /Prim1 is in a partially edited state as we weren't
        # able to move the the local RenamedChild specs back to Child while the
        # prim was unloaded.
        #
        # The now loaded /Prim3 is unchanged from its unloaded state as the
        # payload path still points to /Ref/RenamedChild/GrandChild which no
        # longer exists at that path (and we have an unresolved payload
        # composition error on this prim now).
        prim1Contents = {
            '.' : ['refAttr', 'localAttr'],
            'Child' : {
                '.' : ['childAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr']
                },
            },
            'RenamedChild' : {
                '.' : ['overChildAttr'],
                'GrandChild' : {
                    '.' : ['overGrandChildAttr'],
                },
            },
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
            'Prim3' : prim3Contents
        })

    def test_BasicNestedReferences(self):
        """Test downstream dependency name space edits across chains of
        references."""

        # Setup:
        # Layer 1 has simple hierarchy of /Ref, Child, GrandChild that is
        # referenced and will be namespace edited.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1.ImportFromString('''#usda 1.0
            def "Ref" {
                int refAttr

                def "Child" {
                    int childAttr

                    def "GrandChild" {
                        int grandChildAttr
                    }
                }
            }
        ''')

        # Layer 2 has three prims that reference /Ref, /Ref/Child, and
        # /Ref/Child/GrandChild from layer 1 respectively. These will be
        # affected by downstream dependency edits.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2.ImportFromString('''#usda 1.0
            def "Prim1" (
                references = @''' + layer1.identifier + '''@</Ref>
            ) {
                over "Child" {
                    int over1ChildAttr
                    over "GrandChild" {
                        int over1GrandChildAttr
                    }
                }
                def "Prim1_LocalChild" {}
                int prim1_localAttr
            }

            def "Prim2" (
                references = @''' + layer1.identifier + '''@</Ref/Child>
            ) {
                over "GrandChild" {
                    int over2GrandChildAttr
                }
                def "Prim2_LocalChild" {}
                int prim2_localAttr
            }

            def "Prim3" (
                references = @''' + layer1.identifier + '''@</Ref/Child/GrandChild>
            ) {
                def "Prim3_LocalChild" {}
                int prim3_LocalAttr
            }
        ''')

        # Layer3 has multiple prims that each reference the possible
        # prims in layer2 in order to test the various permutations of direct
        # and ancestral reference chains.
        # /Prim4 : References /Prim1 which directly references /Ref
        # /Prim4_A : References /Prim1/Child which ancestrally references
        #            /Ref/Child
        # /Prim4_B : References /Prim1/Child/GrandChild which ancestrally
        #            references /Ref/Child/GrandChild
        # /Prim5 : References /Prim2 which directly references /Ref/Child
        # /Prim5_A : References /Prim2/GrandChild which ancestrally references
        #            /Ref/Child/GrandChild
        # /Prim6 : References /Prim3 which directly references
        #          /Ref/Child/GrandChild
        layer3 = Sdf.Layer.CreateAnonymous("layer3.usda")
        layer3.ImportFromString('''#usda 1.0
            def "Prim4" (
                references = @''' + layer2.identifier + '''@</Prim1>
            ) {
                over "Child" {
                    int over4ChildAttr
                    over "GrandChild" {
                        int over4GrandChildAttr
                    }
                }
                def "LocalChild" {}
                int localAttr
            }

            def "Prim4_A" (
                references = @''' + layer2.identifier + '''@</Prim1/Child>
            ) {
                over "GrandChild" {
                    int over4_AGrandChildAttr
                }
                def "LocalChild" {}
                int localAttr
            }

            def "Prim4_B" (
                references = @''' + layer2.identifier + '''@</Prim1/Child/GrandChild>
            ) {
                def "LocalChild" {}
                int localAttr
            }

            def "Prim5" (
                references = @''' + layer2.identifier + '''@</Prim2>
            ) {
                over "GrandChild" {
                    int over5GrandChildAttr
                }
                def "LocalChild" {}
                int localAttr
            }

            def "Prim5_A" (
                references = @''' + layer2.identifier + '''@</Prim2/GrandChild>
            ) {
                def "LocalChild" {}
                int localAttr
            }

            def "Prim6" (
                references = @''' + layer2.identifier + '''@</Prim3>
            ) {
                def "LocalChild" {}
                int localAttr
            }
        ''')

        # Open all three layers as stages.
        stage1 = Usd.Stage.Open(layer1, Usd.Stage.LoadAll)
        stage2 = Usd.Stage.Open(layer2, Usd.Stage.LoadAll)
        stage3 = Usd.Stage.Open(layer3, Usd.Stage.LoadAll)

        # Create an editor for stage1 with ONLY stage 3 as a dependent stage
        # (i.e. excluding stage 2). This will still cause edits to layer2 when
        # layer1 is edited as stage3 has dependencies on layer 2 and thus, these
        # changes will be reflected in stage 2.
        editor = Usd.NamespaceEditor(stage1)
        editor.AddDependentStage(stage3)

        # Verify the initial composition fields for each layer.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer1), {})

        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'references' : (Sdf.Reference(layer1.identifier, '/Ref'),)
            },
            '/Prim2' : {
                'references' : (Sdf.Reference(layer1.identifier, '/Ref/Child'),)
            },
            '/Prim3' : {
                'references' : (Sdf.Reference(layer1.identifier,
                                              '/Ref/Child/GrandChild'),)
            },
        })

        self.assertEqual(self._GetCompositionFieldsInLayer(layer3), {
            '/Prim4' : {
                'references' : (Sdf.Reference(layer2.identifier, '/Prim1'),)
            },
            '/Prim4_A' : {
                'references' : (Sdf.Reference(layer2.identifier, '/Prim1/Child'),)
            },
            '/Prim4_B' : {
                'references' : (Sdf.Reference(layer2.identifier,
                                              '/Prim1/Child/GrandChild'),)
            },
            '/Prim5' : {
                'references' : (Sdf.Reference(layer2.identifier, '/Prim2'),)
            },
            '/Prim5_A' : {
                'references' : (Sdf.Reference(layer2.identifier,
                                              '/Prim2/GrandChild'),)
            },
            '/Prim6' : {
                'references' : (Sdf.Reference(layer2.identifier, '/Prim3'),)
            },
        })

        # Verify the initial contents of stage 1.
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
                'Child' : {
                    '.' : ['childAttr'],
                    'GrandChild' : {
                        '.' : ['grandChildAttr']
                    }
                }
            },
        })

        # Verify the initial contents of stage 2 with its three single
        # reference composed prims
        prim1Contents = {
            '.' : ['refAttr', 'prim1_localAttr'],
            'Child' : {
                '.' : ['childAttr', 'over1ChildAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr', 'over1GrandChildAttr']
                },
            },
            'Prim1_LocalChild' : {},
        }

        prim2Contents =  {
            '.' : ['childAttr', 'prim2_localAttr'],
            'GrandChild' : {
                '.' : ['grandChildAttr', 'over2GrandChildAttr']
            },
            'Prim2_LocalChild' : {},
        }

        prim3Contents = {
            '.' : ['grandChildAttr', 'prim3_LocalAttr'],
            'Prim3_LocalChild' : {},
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
            'Prim3' : prim3Contents
        })

        # Verify the initial contents of stage 3 with its all its prims composed
        # from 2-chain references
        prim4Contents = {
            '.' : ['refAttr', 'prim1_localAttr', 'localAttr'],
            'Child' : {
                '.' : ['childAttr', 'over1ChildAttr', 'over4ChildAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr', 'over1GrandChildAttr',
                           'over4GrandChildAttr']
                },
            },
            'Prim1_LocalChild' : {},
            'LocalChild' : {},
        }

        prim4_AContents = {
            '.' : ['childAttr', 'over1ChildAttr', 'localAttr'],
            'GrandChild' : {
                '.' : ['grandChildAttr', 'over1GrandChildAttr',
                       'over4_AGrandChildAttr']
            },
            'LocalChild' : {},
        }

        prim4_BContents = {
            '.' : ['grandChildAttr', 'over1GrandChildAttr', 'localAttr'],
            'LocalChild' : {},
        }

        prim5Contents =  {
            '.' : ['childAttr', 'prim2_localAttr', 'localAttr'],
            'GrandChild' : {
                '.' : ['grandChildAttr', 'over2GrandChildAttr',
                       'over5GrandChildAttr']
            },
            'Prim2_LocalChild' : {},
            'LocalChild' : {},
        }

        prim5_AContents =  {
            '.' : ['grandChildAttr', 'over2GrandChildAttr', 'localAttr'],
            'LocalChild' : {},
        }

        prim6Contents = {
            '.' : ['grandChildAttr', 'prim3_LocalAttr', 'localAttr'],
            'Prim3_LocalChild' : {},
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage3, {
            'Prim4' : prim4Contents,
            'Prim4_A' : prim4_AContents,
            'Prim4_B' : prim4_BContents,
            'Prim5' : prim5Contents,
            'Prim5_A' : prim5_AContents,
            'Prim6' : prim6Contents
        })

        # Edit: Rename /Ref/Child to /Ref/RenamedChild on stage 1.
        with self.ApplyEdits(editor, "Move /Ref/Child -> /Ref/RenamedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/Child', '/Ref/RenamedChild'))

        # Verify the udpated composition fields in layer2. The reference fields
        # to /Ref/Child and its descendant /Ref/Child/GrandChild have been
        # updated to use the renamed paths.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'references' : (Sdf.Reference(layer1.identifier, '/Ref'),)
            },
            '/Prim2' : {
                'references' : (Sdf.Reference(layer1.identifier,
                                              '/Ref/RenamedChild'),)
            },
            '/Prim3' : {
                'references' : (Sdf.Reference(layer1.identifier,
                                              '/Ref/RenamedChild/GrandChild'),)
            },
        })

        # Verify the udpated composition fields in layer3. The reference
        # fields to /Prim1/Child and its descendant /Prim1/Child/GrandChild have
        # to be updated as all the specs composing /Prim1/Child are renamed to 
        # RenamedChild by the dependent edits.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer3), {
            '/Prim4' : {
                'references' : (Sdf.Reference(layer2.identifier, '/Prim1'),)
            },
            '/Prim4_A' : {
                'references' : (Sdf.Reference(layer2.identifier,
                                              '/Prim1/RenamedChild'),)
            },
            '/Prim4_B' : {
                'references' : (Sdf.Reference(layer2.identifier,
                                              '/Prim1/RenamedChild/GrandChild'),)
            },
            '/Prim5' : {
                'references' : (Sdf.Reference(layer2.identifier, '/Prim2'),)
            },
            '/Prim5_A' : {
                'references' : (Sdf.Reference(layer2.identifier,
                                              '/Prim2/GrandChild'),)
            },
            '/Prim6' : {
                'references' : (Sdf.Reference(layer2.identifier, '/Prim3'),)
            },
        })

        # Verify the simple prim rename is reflected in contents of stage 1.
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
                'RenamedChild' : {
                    '.' : ['childAttr'],
                    'GrandChild' : {
                        '.' : ['grandChildAttr']
                    }
                }
            },
        })

        # Verify the udpated contents of stage 2.
        #
        # There are prims in stage 3 that reference /Prim1/Child on layer2 so
        # which ancestrally references /Ref/Child. So the specs for /Prim1/Child
        # on layer 2 are renamed to RenamedChild for this dependency. This
        # change is reflected in the contents of stage 2.
        #
        # Stage 3 also has prims with dependencies on /Prim2 and /Prim3, but
        # the contents of those two prims remain the same as the reference paths
        # are just updated to /Ref/RenamedChild and /Ref/RenamedChild/GrandChild.
        prim1Contents = {
            '.' : ['refAttr', 'prim1_localAttr'],
            'RenamedChild' : {
                '.' : ['childAttr', 'over1ChildAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr', 'over1GrandChildAttr']
                },
            },
            'Prim1_LocalChild' : {},
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
            'Prim3' : prim3Contents
        })

        # Verify the updated contents of stage 3.
        #
        # /Prim4 references /Prim1 which references /Ref so /Prim4/Child is also
        # renamed to /Prim4/RenamedChild.
        # No other prims' contents change as they each had one of their nested
        # references arcs updated to the new path.
        prim4Contents = {
            '.' : ['refAttr', 'prim1_localAttr', 'localAttr'],
            'RenamedChild' : {
                '.' : ['childAttr', 'over1ChildAttr', 'over4ChildAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr', 'over1GrandChildAttr',
                           'over4GrandChildAttr']
                },
            },
            'Prim1_LocalChild' : {},
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage3, {
            'Prim4' : prim4Contents,
            'Prim4_A' : prim4_AContents,
            'Prim4_B' : prim4_BContents,
            'Prim5' : prim5Contents,
            'Prim5_A' : prim5_AContents,
            'Prim6' : prim6Contents
        })

        # Edit: Reparent and rename /Ref/RenamedChild to /MovedChild
        with self.ApplyEdits(editor, "Move /Ref/RenamedChild -> /MovedChild"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/RenamedChild', '/MovedChild'))

        # Verify the udpated composition fields in layer2. The reference fields
        # to /Ref/RenamedChild and its descendant /Ref/RenamedChild/GrandChild
        # have been updated to use the moved paths.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'references' : (Sdf.Reference(layer1.identifier, '/Ref'),)
            },
            '/Prim2' : {
                'references' : (Sdf.Reference(layer1.identifier, '/MovedChild'),)
            },
            '/Prim3' : {
                'references' : (Sdf.Reference(layer1.identifier,
                                              '/MovedChild/GrandChild'),)
            },
        })

        # Verify the udpated composition fields in layer3. Like with the
        # previous edit, the reference fields to /Prim1/RenamedChild and
        # its descendant /Prim1/RenamedChild/GrandChild have to be updated as
        # all the specs composing /Prim1/RenameChild have been moved by the 
        # dependent edits. However, since RenamedChild was moved out /Ref, it no
        # longer maps across the reference to /Prim1 which results in the 
        # deletion of the composed /Prim1/RenamedChild. So the reference fields
        # for /Prim4_A and /Prim4_B are updated to delete these references to
        # prims that no longer exist in layer2.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer3), {
            '/Prim4' : {
                'references' : (Sdf.Reference(layer2.identifier, '/Prim1'),)
            },
            '/Prim4_A' : {
                'references' : ()
            },
            '/Prim4_B' : {
                'references' : ()
            },
            '/Prim5' : {
                'references' : (Sdf.Reference(layer2.identifier, '/Prim2'),)
            },
            '/Prim5_A' : {
                'references' : (Sdf.Reference(layer2.identifier,
                                              '/Prim2/GrandChild'),)
            },
            '/Prim6' : {
                'references' : (Sdf.Reference(layer2.identifier, '/Prim3'),)
            },
        })

        # Verify the simple reparent and rename is reflected in contents of
        # stage 1.
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
            },
            'MovedChild' : {
                '.' : ['childAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr']
                }
            }
        })

        # Verify the udpated contents of stage 2.
        #
        # There are prims in stage 3 that reference /Prim1/RenamedChild on
        # layer2 which ancestrally references /Ref/RenamedChild. So the specs
        # for /Prim1/RenamedChild on layer 2 are deleted for this dependency
        # (since the RenamedChild was moved out of the reference). This change
        # is reflected in the contents of stage 2.
        #
        # Stage 3 also has prims with dependencies on /Prim2 and /Prim3, but
        # these contents remain the same as the reference paths are
        # just updated to /MovedChild and /MovedChild/GrandChild.
        prim1Contents = {
            '.' : ['refAttr', 'prim1_localAttr'],
            'Prim1_LocalChild' : {},
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
            'Prim3' : prim3Contents
        })

        # Verify the updated contents of stage 3.
        #
        # /Prim4 references /Prim1 which references /Ref so /Prim4/RenamedChild
        # is also deleted. /Prim4_A and /Prim4_B have lost their references so
        # these also only have the contents of their local specs.
        #
        # The rest of the prims have no contents change as they each had one of
        # their nested references arcs updated to the new path.
        prim4Contents = {
            '.' : ['refAttr', 'prim1_localAttr', 'localAttr'],
            'Prim1_LocalChild' : {},
            'LocalChild' : {},
        }

        prim4_AContents = {
            '.' : ['localAttr'],
            'LocalChild' : {},
        }

        prim4_BContents = {
            '.' : ['localAttr'],
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage3, {
            'Prim4' : prim4Contents,
            'Prim4_A' : prim4_AContents,
            'Prim4_B' : prim4_BContents,
            'Prim5' : prim5Contents,
            'Prim5_A' : prim5_AContents,
            'Prim6' : prim6Contents
        })

        # Edit: Reparent and rename /MovedChild back to its original path
        # /Ref/Child
        with self.ApplyEdits(editor, "Move /MovedChild -> /Ref/Child"):
            self.assertTrue(editor.MovePrimAtPath('/MovedChild', '/Ref/Child'))

        # Verify the udpated composition fields in layer2. The reference fields
        # have all been updated to their original paths.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'references' : (Sdf.Reference(layer1.identifier, '/Ref'),)
            },
            '/Prim2' : {
                'references' : (Sdf.Reference(layer1.identifier, '/Ref/Child'),)
            },
            '/Prim3' : {
                'references' : (Sdf.Reference(layer1.identifier,
                                              '/Ref/Child/GrandChild'),)
            },
        })

        # Verify the udpated composition fields in layer3 which have NOT changed
        # because the prior edit deleted both references that initially
        # referred to /Prim1/Child.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer3), {
            '/Prim4' : {
                'references' : (Sdf.Reference(layer2.identifier, '/Prim1'),)
            },
            '/Prim4_A' : {
                'references' : ()
            },
            '/Prim4_B' : {
                'references' : ()
            },
            '/Prim5' : {
                'references' : (Sdf.Reference(layer2.identifier, '/Prim2'),)
            },
            '/Prim5_A' : {
                'references' : (Sdf.Reference(layer2.identifier, '/Prim2/GrandChild'),)
            },
            '/Prim6' : {
                'references' : (Sdf.Reference(layer2.identifier, '/Prim3'),)
            },
        })

        # Verify the return to the the original contents in stage 1.
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
                'Child' : {
                    '.' : ['childAttr'],
                    'GrandChild' : {
                        '.' : ['grandChildAttr']
                    }
                }
            },
        })

        # Verify the udpated contents of stage 2.
        #
        # The contents of /Prim1 have returned to the original contents with
        # the exception that the local opinions for Child and GrandChild in 
        # layer2 were deleted in the prior edit and cannot be restored.
        #
        # /Prim2 and /Prim3 remain unchanged.
        prim1Contents = {
            '.' : ['refAttr', 'prim1_localAttr'],
            'Child' : {
                '.' : ['childAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr']
                },
            },
            'Prim1_LocalChild' : {},
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
            'Prim3' : prim3Contents
        })

        # Verify the updated contents of stage 3.
        #
        # /Prim4 returns to its original contents with the exception that
        # deleted local opinions on layer3 for Child and the deleted opinions
        # from /Prim1/Child on layer2 cannot be restored.
        #
        # /Prim4_A and /Prim4_B remain unchanged (and therefore unrestored)
        # because their references were deleted by the previous edit.
        #
        # The rest of the prims continue to have no contents change as they each
        # had one of their nested references arcs updated to the original path.
        prim4Contents = {
            '.' : ['refAttr', 'prim1_localAttr', 'localAttr'],
            'Child' : {
                '.' : ['childAttr'],
                'GrandChild' : {
                    '.' : ['grandChildAttr']
                },
            },
            'Prim1_LocalChild' : {},
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage3, {
            'Prim4' : prim4Contents,
            'Prim4_A' : prim4_AContents,
            'Prim4_B' : prim4_BContents,
            'Prim5' : prim5Contents,
            'Prim5_A' : prim5_AContents,
            'Prim6' : prim6Contents
        })

        # Edit: Delete /Ref/Child
        with self.ApplyEdits(editor, "Delete /Ref/Child"):
            self.assertTrue(editor.DeletePrimAtPath('/Ref/Child'))

        # Verify the udpated composition fields in layer2. The reference fields
        # to /Ref/Child and its descendant /Ref/Child/GrandChild have been
        # set to empty now that those prims don't exist on layer1
        self.assertEqual(self._GetCompositionFieldsInLayer(layer2), {
            '/Prim1' : {
                'references' : (Sdf.Reference(layer1.identifier, '/Ref'),)
            },
            '/Prim2' : {
                'references' : ()
            },
            '/Prim3' : {
                'references' : ()
            },
        })

        # Verify the udpated composition fields in layer3. The only change now
        # is that /Prim5_A's reference to /Prim2/GrandChild has been deleted as
        # /Prim2/GrandChild is no a valid composed prim of layer2.
        self.assertEqual(self._GetCompositionFieldsInLayer(layer3), {
            '/Prim4' : {
                'references' : (Sdf.Reference(layer2.identifier, '/Prim1'),)
            },
            '/Prim4_A' : {
                'references' : ()
            },
            '/Prim4_B' : {
                'references' : ()
            },
            '/Prim5' : {
                'references' : (Sdf.Reference(layer2.identifier, '/Prim2'),)
            },
            '/Prim5_A' : {
                'references' : ()
            },
            '/Prim6' : {
                'references' : (Sdf.Reference(layer2.identifier, '/Prim3'),)
            },
        })

        # Verify the deletion of Child on stage 1.
        self._VerifyStageContents(stage1, {
            'Ref': {
                '.' : ['refAttr'],
            },
        })

        # Verify the udpated contents of stage 2.
        #
        # The contents of /Prim1 no longer have the child Child.
        #
        # /Prim2 and /Prim3 have lost their references so only have local
        # opinions from layer2.
        prim1Contents = {
            '.' : ['refAttr', 'prim1_localAttr'],
            'Prim1_LocalChild' : {},
        }

        prim2Contents =  {
            '.' : ['prim2_localAttr'],
            'Prim2_LocalChild' : {},
        }

        prim3Contents = {
            '.' : ['prim3_LocalAttr'],
            'Prim3_LocalChild' : {},
        }

        self._VerifyStageContents(stage2, {
            'Prim1' : prim1Contents,
            'Prim2' : prim2Contents,
            'Prim3' : prim3Contents
        })

        # Verify the updated contents of stage 3.
        #
        # /Prim4 loses all opinions about Child.
        #
        # /Prim4_A and /Prim4_B remain unchanged still because of the prior
        # deleted references.
        #
        # /Prim5 and /Prim6 are affected by the deletion of the references on
        # /Prim2 and /Prim3 respectively, but they still composed the layer2
        # local opinions from /Prim2 and /Prim3
        #
        # /Prim5_A only has its own local opinions because its own reference
        # has also been deleted.
        prim4Contents = {
            '.' : ['refAttr', 'prim1_localAttr', 'localAttr'],
            'Prim1_LocalChild' : {},
            'LocalChild' : {},
        }

        prim5Contents =  {
            '.' : ['prim2_localAttr', 'localAttr'],
            'Prim2_LocalChild' : {},
            'LocalChild' : {},
        }

        prim5_AContents =  {
            '.' : ['localAttr'],
            'LocalChild' : {},
        }

        prim6Contents = {
            '.' : ['prim3_LocalAttr', 'localAttr'],
            'Prim3_LocalChild' : {},
            'LocalChild' : {},
        }

        self._VerifyStageContents(stage3, {
            'Prim4' : prim4Contents,
            'Prim4_A' : prim4_AContents,
            'Prim4_B' : prim4_BContents,
            'Prim5' : prim5Contents,
            'Prim5_A' : prim5_AContents,
            'Prim6' : prim6Contents
        })

if __name__ == '__main__':
    unittest.main()
