// DepthMesh.cpp : Renders the depth image of the Kinect as mesh
//

#include "stdafx.h"
#include "DepthMesh.h"

#include "GraphicsContext.h"
#include "Kinect.h"

DepthMesh::DepthMesh(_In_ GraphicsContext & DeviceContext)
	:PlaneMesh(DeviceContext)
{
}

void DepthMesh::Create(_In_ Kinect & Kinect)
{
	PlaneMesh.CreatePlane(Kinect::DepthImageWidth, Kinect::DepthImageHeigth);

	Instances.push_back(Transform(Kinect.GetOffset(), Quaternion(), Vector3(Kinect.GetRealWorldToVirutalScale(), Kinect.GetRealWorldToVirutalScale(), -Kinect.GetRealWorldToVirutalScale())));
	Kinect.DepthVerticesUpdated += std::make_pair(this, &DepthMesh::DepthVerticesUpdatedCallback);
}

RenderContext::ObjectList DepthMesh::GetRenderObjectList() const
{
	return RenderContext::ObjectList( PlaneMesh, Instances );
}

void DepthMesh::DepthVerticesUpdatedCallback(_In_ const Kinect::CameraSpacePointList & DepthVertices)
{
	VertexCache.resize(DepthVertices.size());

	std::transform(DepthVertices.begin(), DepthVertices.end(), VertexCache.begin(), [](auto Vertex) { return Mesh::Vertex({ { Vertex.X, Vertex.Y, Vertex.Z },{ 0.0f, 0.0f, 0.0f, 0.0f } }); });

	PlaneMesh.UpdateVertices(VertexCache);
}
