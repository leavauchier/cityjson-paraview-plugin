#include "vtkCityJSONFeature.h"

// VTK Includes
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkLine.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyLine.h"
#include "vtkPolygon.h"

#include <sstream>
#include <string>
#include <vtkStringArray.h>

vtkStandardNewMacro(vtkCityJSONFeature);

namespace {
    vtkOStreamWrapper &operator<<(vtkOStreamWrapper &os, const Json::Value &root) {
        Json::StreamWriterBuilder builder;
        builder["commentStyle"] = "All";
        builder["indentation"] = "  ";
        std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

        writer->write(root, &os.GetOStream());
        return os;
    }
}

vtkCityJSONFeature::vtkCityJSONFeature() = default;

//----------------------------------------------------------------------------
vtkCityJSONFeature::~vtkCityJSONFeature() = default;

// Get boundaries for each object and insert polygons that reference each inserted point
vtkPolyData *vtkCityJSONFeature::ConnectTheDots(const Json::Value &cityObject, vtkPolyData *outputData) {

    if (cityObject.isNull()){
        vtkErrorMacro(<< "Geometry node is missing!");
        return nullptr;
    }

    // Get all polygons currently present
    vtkCellArray *polys = outputData->GetPolys();

    // Store type of this object for later use
    std::string objectType = cityObject["type"].asString();

    for (Json::Value geometry : cityObject["geometry"]) {
        for (Json::Value boundary : geometry["boundaries"]) {
            for (Json::Value element : boundary) {
                for (Json::Value vertices : element) {
                    // Sometimes vertices is a layer shallower than other times, so go back a layer in that case
                    if (vertices.isInt()) {
                        vertices = element;
                    }

                    if (vertices.empty()) {
                        continue;
                    }

                    vtkNew<vtkPolygon> poly;
                    vtkIdList *polyPointIds = poly->GetPointIds();

                    // Set the object type of this polygon to the 'type' taken from CityJSON file
                    vtkAbstractArray *array = outputData->GetCellData()->GetAbstractArray("object-type");
                    auto *ids = vtkArrayDownCast<vtkStringArray>(array);
                    ids->InsertNextValue(objectType);

                    // For each vertex id in the boundary list, insert and add to poly
                    for (Json::Value boundaryVertexId : vertices) {
                        polyPointIds->InsertNextId(boundaryVertexId.asInt());
                    }

                    // Define the number of points in this polygon and insert the polygon into the polys array
                    polyPointIds->SetNumberOfIds(vertices.size());
                    polys->InsertNextCell(poly);
                }
            }
        }
    }

    return outputData;
}

// Extract all vertices and insert them as points
void vtkCityJSONFeature::ExtractVertices(const Json::Value &vertices, vtkPolyData *outputData) {

    vtkPoints *points = outputData->GetPoints();

    // For each CityJSON point, insert it in order so point ordering is maintained
    for (Json::Value vertex : vertices) {
        points->InsertNextPoint(vertex[0].asDouble(), vertex[1].asDouble(), vertex[2].asDouble());
    }

}

//----------------------------------------------------------------------------
void vtkCityJSONFeature::PrintSelf(ostream &os, vtkIndent indent) {
    Superclass::PrintSelf(os, indent);
    os << indent << "vtkCityJSONFeature" << std::endl;
    os << indent << "Root: ";

    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "All";
    builder["indentation"] = "  ";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    writer->write(this->featureRoot, &os);
}
