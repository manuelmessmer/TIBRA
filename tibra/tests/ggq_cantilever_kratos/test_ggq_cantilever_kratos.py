# Project imports
import TIBRA_PythonApplication as TIBRA_APP
from tibra.python_scripts.helper import *

try:
    import KratosMultiphysics as KM
    kratos_available = True
except:
    print("KratosMultiphysics is not available")
    kratos_available = False

if kratos_available:
    import json
    from kratos_interface.kratos_analysis import Analysis
    from kratos_interface.bounding_box_bcs import DirichletCondition
    from kratos_interface.bounding_box_bcs import NeumannCondition

# STL imports
import unittest
import numpy as np

def run_analysis(number_cross_elements, number_z_elements, reduction_flag, polynomial_degree):
    if kratos_available:
        parameters = ReadParameters("tibra/tests/ggq_cantilever_kratos/TIBRAParameters.json")

        parameters.Set("number_of_knot_spans", [number_cross_elements, number_cross_elements, number_z_elements])
        parameters.Set("polynomial_order", polynomial_degree)
        if reduction_flag == False:
            parameters.Set("integration_method", "Gauss")
        else:
            parameters.Set("integration_method", "ReducedExact")

        embedder = TIBRA_APP.TIBRA(parameters)

        points_all = TIBRA_APP.IntegrationPointVector()
        elements = embedder.GetElements()

        for element in elements:
            if element.IsTrimmed():
                for point_trimmed_reduced in element.GetIntegrationPoints():
                    if(point_trimmed_reduced.GetWeight() > 0.0):
                        points_all.append(point_trimmed_reduced)
            else:
                for point_inside in element.GetIntegrationPoints():
                    points_all.append(point_inside)

        p = 100
        boundary_condition = []
        boundary_condition.append( DirichletCondition([-100, -100, -0.01], [100, 100, 0.01], [1,1,1]) )
        boundary_condition.append( NeumannCondition([-100, -100, 9.99], [100, 100, 10.01], [0, p, 0]) )


        kratos_settings_filename = "tibra/tests/ggq_cantilever_kratos/KratosParameters.json"
        analysis = Analysis(parameters, kratos_settings_filename, points_all, boundary_condition)
        model_part = analysis.GetModelPart()
        geometry = model_part.GetGeometry("NurbsVolume")

        number_of_elements = model_part.NumberOfElements()
        param = KM.Vector(3)
        param[0] = 0.5
        param[1] = 0.5
        param[2] = 1
        coord = geometry.GlobalCoordinates(param)
        disp_simulation = coord[1]-1

        return [disp_simulation, number_of_elements]

class TestGGQCantileverKratos(unittest.TestCase):
    def compare_full_vs_reduced_p_2(self, number_knotspans):
        number_knot_spans_cross = 2
        [disp_reduced, n_elements_reduced] = run_analysis(number_knot_spans_cross, number_knotspans, True,[2,2,2])
        [disp_full, n_elements_full] = run_analysis(number_knot_spans_cross, number_knotspans, False,[2,2,2])

        n_elements_full_ref = (number_knot_spans_cross*3) * (number_knot_spans_cross*3) * (number_knotspans*3)
        order = 4
        continuity = 0
        red_continuity = order - continuity
        ndof = (order+1)*number_knotspans - (order-red_continuity+1)*(number_knotspans-1)
        ndof_2 = (order+1)*number_knot_spans_cross - (order-red_continuity+1)*(number_knot_spans_cross-1)
        n_elements_reduced_ref = np.ceil(ndof_2/2) * np.ceil(ndof_2/2) * np.ceil(ndof/2)

        self.assertEqual(n_elements_full, n_elements_full_ref)
        self.assertEqual(n_elements_reduced, n_elements_reduced_ref)

        print("disp_reduced: ", disp_reduced)
        print("disp_full: ", disp_full)
        print( "Rel error: ", (disp_reduced-disp_full)/disp_full )
        self.assertAlmostEqual(disp_reduced, disp_full, 12)

    def compare_full_vs_reduced_p_3(self, number_knotspans):
        number_knot_spans_cross = 1
        [disp_reduced, n_elements_reduced] = run_analysis(number_knot_spans_cross, number_knotspans, True,[3,3,3])
        [disp_full, n_elements_full] = run_analysis(number_knot_spans_cross, number_knotspans, False,[3,3,3])

        n_elements_full_ref = (number_knot_spans_cross*4) * (number_knot_spans_cross*4) * (number_knotspans*4)
        order = 6
        continuity = 1
        red_continuity = order - continuity
        ndof = (order+1)*number_knotspans - (order-red_continuity+1)*(number_knotspans-1)
        ndof_2 = (order+1)*number_knot_spans_cross - (order-red_continuity+1)*(number_knot_spans_cross-1)
        n_elements_reduced_ref = np.ceil(ndof_2/2) * np.ceil(ndof_2/2) * np.ceil(ndof/2)

        self.assertEqual(n_elements_full, n_elements_full_ref)
        self.assertEqual(n_elements_reduced, n_elements_reduced_ref)
        self.assertAlmostEqual(disp_reduced, disp_full, 11)

    def compare_full_vs_reduced_p_4(self, number_knotspans):
        number_knot_spans_cross = 1
        [disp_reduced, n_elements_reduced] = run_analysis(number_knot_spans_cross, number_knotspans, True,[4,4,4])
        [disp_full, n_elements_full] = run_analysis(number_knot_spans_cross, number_knotspans, False,[4,4,4])

        n_elements_full_ref = (number_knot_spans_cross*5) * (number_knot_spans_cross*5) * (number_knotspans*5)
        order = 8
        continuity = 2
        red_continuity = order - continuity
        ndof = (order+1)*number_knotspans - (order-red_continuity+1)*(number_knotspans-1)
        ndof_2 = (order+1)*number_knot_spans_cross - (order-red_continuity+1)*(number_knot_spans_cross-1)
        n_elements_reduced_ref = np.ceil(ndof_2/2) * np.ceil(ndof_2/2) * np.ceil(ndof/2)

        self.assertEqual(n_elements_full, n_elements_full_ref)
        self.assertEqual(n_elements_reduced, n_elements_reduced_ref)
        print( "Rel error: ", (disp_reduced-disp_full)/disp_full )
        self.assertAlmostEqual(disp_reduced, disp_full, 10)

    def test_1_knotspans(self):
        self.compare_full_vs_reduced_p_2(1)
        self.compare_full_vs_reduced_p_3(1)
        self.compare_full_vs_reduced_p_4(1)

    def test_2_knotspans(self):
        self.compare_full_vs_reduced_p_2(2)
        self.compare_full_vs_reduced_p_3(2)
        self.compare_full_vs_reduced_p_4(2)

    def test_3_knotspans(self):
        self.compare_full_vs_reduced_p_2(3)
        self.compare_full_vs_reduced_p_3(3)
        self.compare_full_vs_reduced_p_4(3)

    def test_4_knotspans(self):
        self.compare_full_vs_reduced_p_2(4)
        self.compare_full_vs_reduced_p_3(4)
        self.compare_full_vs_reduced_p_4(4)

    def test_5_knotspans(self):
        self.compare_full_vs_reduced_p_2(5)
        self.compare_full_vs_reduced_p_3(5)
        self.compare_full_vs_reduced_p_4(5)

    # def test_6_knotspans(self):
    #     self.compare_full_vs_reduced_p_2(6)
    #     self.compare_full_vs_reduced_p_3(6)
    #     self.compare_full_vs_reduced_p_4(6)

    # def test_7_knotspans(self):
    #     self.compare_full_vs_reduced_p_2(7)
    #     self.compare_full_vs_reduced_p_3(7)
    #     self.compare_full_vs_reduced_p_4(7)

    # def test_8_knotspans(self):
    #     self.compare_full_vs_reduced_p_2(8)
    #     self.compare_full_vs_reduced_p_3(8)
    #     self.compare_full_vs_reduced_p_4(8)

    # def test_9_knotspans(self):
    #     self.compare_full_vs_reduced_p_2(9)
    #     self.compare_full_vs_reduced_p_3(9)
    #     self.compare_full_vs_reduced_p_4(9)

    # def test_10_knotspans(self):
    #     self.compare_full_vs_reduced_p_2(10)
    #     self.compare_full_vs_reduced_p_3(10)
    #     self.compare_full_vs_reduced_p_4(10)

    # def test_11_knotspans(self):
    #     self.compare_full_vs_reduced_p_2(11)
    #     self.compare_full_vs_reduced_p_3(11)
    #     self.compare_full_vs_reduced_p_4(11)

    # def test_12_knotspans(self):
    #     self.compare_full_vs_reduced_p_2(12)
    #     self.compare_full_vs_reduced_p_3(12)
    #     self.compare_full_vs_reduced_p_4(12)

    # def test_13_knotspans(self):
    #     self.compare_full_vs_reduced_p_2(13)
    #     self.compare_full_vs_reduced_p_3(13)
    #     self.compare_full_vs_reduced_p_4(13)

    # def test_14_knotspans(self):
    #     self.compare_full_vs_reduced_p_2(14)
    #     self.compare_full_vs_reduced_p_3(14)
    #     self.compare_full_vs_reduced_p_4(14)

    # def test_15_knotspans(self):
    #     self.compare_full_vs_reduced_p_2(15)
    #     self.compare_full_vs_reduced_p_3(15)
    #     self.compare_full_vs_reduced_p_4(15)

    # def test_16_knotspans(self):
    #     self.compare_full_vs_reduced_p_2(16)
    #     self.compare_full_vs_reduced_p_3(16)
    #     self.compare_full_vs_reduced_p_4(16)

    # def test_17_knotspans(self):
    #     self.compare_full_vs_reduced_p_2(17)
    #     self.compare_full_vs_reduced_p_3(17)
    #     self.compare_full_vs_reduced_p_4(17)

    # def test_18_knotspans(self):
    #     self.compare_full_vs_reduced_p_2(18)
    #     self.compare_full_vs_reduced_p_3(18)
    #     self.compare_full_vs_reduced_p_4(18)

    # def test_19_knotspans(self):
    #     self.compare_full_vs_reduced_p_2(19)
    #     self.compare_full_vs_reduced_p_3(19)
    #     self.compare_full_vs_reduced_p_4(19)

    # def test_20_knotspans(self):
    #     self.compare_full_vs_reduced_p_2(20)
    #     self.compare_full_vs_reduced_p_3(20)
    #     self.compare_full_vs_reduced_p_4(20)

if __name__ == "__main__":
    unittest.main()
