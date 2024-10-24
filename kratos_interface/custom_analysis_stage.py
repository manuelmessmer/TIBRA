import QuESo_PythonApplication as QuESo_App
# Import Kratos
import KratosMultiphysics as KM
import KratosMultiphysics.IgaApplication as IgaApplication
from KratosMultiphysics.StructuralMechanicsApplication.structural_mechanics_analysis import StructuralMechanicsAnalysis
from kratos_interface.model_part_utilities import ModelPartUtilities

class CustomAnalysisStage(StructuralMechanicsAnalysis):
    """Customized Kratos Analysis Stage.

    Overrides the StructuralMechanicsAnalysis Stage from Kratos.
    """
    def __init__(self, model, queso_settings, kratos_settings_filename, elements, boundary_conditions):
        """The constructor."""
        # Read kratos settings
        with open(kratos_settings_filename,'r') as parameter_file:
            analysis_parameters = KM.Parameters(parameter_file.read())

        self.boundary_conditions = boundary_conditions
        self.elements = elements
        self.queso_settings = queso_settings

        self.lagrange_dofs_required = False
        for condition_param in self.queso_settings.GetList("conditions_settings_list"):
            if( condition_param.GetString("condition_type") == "LagrangeSupportCondition" ):
                self.lagrange_dofs_required = True

        nurbs_model_part = model.CreateModelPart("NurbsMesh")
        nurbs_model_part.AddNodalSolutionStepVariable(KM.DISPLACEMENT)
        nurbs_model_part.AddNodalSolutionStepVariable(KM.REACTION)

        if self.lagrange_dofs_required:
            nurbs_model_part.AddNodalSolutionStepVariable(KM.VECTOR_LAGRANGE_MULTIPLIER)
            nurbs_model_part.AddNodalSolutionStepVariable(IgaApplication.VECTOR_LAGRANGE_MULTIPLIER_REACTION)

        grid_settings = self.queso_settings["background_grid_settings"]
        #Override the NurbsGeometryModeler input parameters
        for modeler in analysis_parameters["modelers"].values():
            if modeler["modeler_name"].GetString() == "NurbsGeometryModeler":
                parameters = modeler["Parameters"]
                parameters.AddEmptyValue("lower_point_xyz")
                parameters["lower_point_xyz"].SetVector(grid_settings.GetDoubleVector("lower_bound_xyz"))
                parameters.AddEmptyValue("upper_point_xyz")
                parameters["upper_point_xyz"].SetVector(grid_settings.GetDoubleVector("upper_bound_xyz"))

                parameters.AddEmptyValue("lower_point_uvw")
                parameters["lower_point_uvw"].SetVector(grid_settings.GetDoubleVector("lower_bound_uvw"))
                parameters.AddEmptyValue("upper_point_uvw")
                parameters["upper_point_uvw"].SetVector(grid_settings.GetDoubleVector("upper_bound_uvw"))

                parameters.AddEmptyValue("polynomial_order")
                parameters["polynomial_order"].SetVector(grid_settings.GetIntVector("polynomial_order"))
                parameters.AddEmptyValue("number_of_knot_spans")
                parameters["number_of_knot_spans"].SetVector(grid_settings.GetIntVector("number_of_elements"))

        self.Initialized = False
        super().__init__(model, analysis_parameters)


    def _ModelersSetupModelPart(self):
        """Override BaseClass to run NURBS modelers."""
        embedded_model_part = self.model.CreateModelPart('EmbeddedModelPart')
        embedded_model_part.AddNodalSolutionStepVariable(KM.DISPLACEMENT)
        embedded_model_part.AddNodalSolutionStepVariable(KM.REACTION)
        embedded_model_part.ProcessInfo.SetValue(KM.DOMAIN_SIZE, 3)
        filename = self.queso_settings["general_settings"].GetString("input_filename")
        self.triangle_mesh = QuESo_App.TriangleMesh()
        QuESo_App.IO.ReadMeshFromSTL(self.triangle_mesh, filename)
        ModelPartUtilities.ReadModelPartFromTriangleMesh(embedded_model_part, self.triangle_mesh)
        # Convert the geometry model or import analysis suitable models.
        for modeler in self._GetListOfModelers():
            if self.echo_level > 1:
                KM.Logger.PrintInfo(self._GetSimulationName(), "Modeler: ", str(modeler), " Setup ModelPart started.")
            modeler.SetupModelPart()
            if self.echo_level > 1:
                KM.Logger.PrintInfo(self._GetSimulationName(), "Modeler: ", str(modeler), " Setup ModelPart finished.")
        return super()._ModelersSetupModelPart()


    def ModifyInitialGeometry(self):
        """Override BaseClass to pass integration points to Kratos."""
        model_part = self.model.GetModelPart('NurbsMesh')
        ModelPartUtilities.RemoveAllElements(model_part)
        ModelPartUtilities.RemoveAllConditions(model_part)
        ModelPartUtilities.AddElementsToModelPart(model_part, self.elements)
        grid_settings = self.queso_settings["background_grid_settings"]
        bounds_xyz = [grid_settings.GetDoubleVector("lower_bound_xyz"),
                      grid_settings.GetDoubleVector("upper_bound_xyz")]
        bounds_uvw = [grid_settings.GetDoubleVector("lower_bound_uvw"),
                      grid_settings.GetDoubleVector("upper_bound_uvw")]
        ModelPartUtilities.AddConditionsToModelPart(model_part, self.boundary_conditions, bounds_xyz, bounds_uvw)

        # Add Dofs
        KM.VariableUtils().AddDof(KM.DISPLACEMENT_X, KM.REACTION_X, model_part)
        KM.VariableUtils().AddDof(KM.DISPLACEMENT_Y, KM.REACTION_Y, model_part)
        KM.VariableUtils().AddDof(KM.DISPLACEMENT_Z, KM.REACTION_Z, model_part)

        if self.lagrange_dofs_required:
            KM.VariableUtils().AddDof(KM.VECTOR_LAGRANGE_MULTIPLIER_X, IgaApplication.VECTOR_LAGRANGE_MULTIPLIER_REACTION_X, model_part)
            KM.VariableUtils().AddDof(KM.VECTOR_LAGRANGE_MULTIPLIER_Y, IgaApplication.VECTOR_LAGRANGE_MULTIPLIER_REACTION_Y, model_part)
            KM.VariableUtils().AddDof(KM.VECTOR_LAGRANGE_MULTIPLIER_Z, IgaApplication.VECTOR_LAGRANGE_MULTIPLIER_REACTION_Z, model_part)


