#ifndef OBJ_LOADER_H_INCLUDED
#define OBJ_LOADER_H_INCLUDED

#include <glm/glm.hpp>
#include <vector>
#include <list>
#include <string>
#include <map>

struct OBJIndex
{
    unsigned int vertexIndex;
    unsigned int uvIndex;
    unsigned int normalIndex;
	unsigned int edgeIndex;
	std::list<OBJIndex>::iterator faceIndex;
	unsigned int tmpIndx;
    bool isEdgeUpdated;
    bool operator<(const OBJIndex& r) const { return vertexIndex < r.vertexIndex; }
	bool operator==(const OBJIndex& r) const { return vertexIndex == r.vertexIndex; }
};

struct IndexedModel
{

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
	std::vector<glm::vec3> colors;
    std::vector<unsigned int> indices;
};

//@Added
struct Edge{
	OBJIndex firstVertex;
	OBJIndex secondVertex;
	float edgeError = 0;
	glm::mat4 edgeQ;
	glm::vec3 newPos;
	
	// Edge struct comperators
	
	bool operator==(const Edge& r) const {
		return (firstVertex.vertexIndex == r.firstVertex.vertexIndex && secondVertex.vertexIndex == r.secondVertex.vertexIndex) || (secondVertex.vertexIndex == r.firstVertex.vertexIndex && firstVertex.vertexIndex == r.secondVertex.vertexIndex);
	}
	bool operator()(const Edge& first, const Edge& second) const {
		return first < second;
	}
	bool operator < (const Edge& second)const	{
		if (firstVertex.vertexIndex < second.firstVertex.vertexIndex) 
			return true;
		else {
			if (firstVertex.vertexIndex == second.firstVertex.vertexIndex) 
				return (secondVertex.vertexIndex < second.secondVertex.vertexIndex);
			else
				return false;
		}
	}
};

// Edges comperator 
struct compEdgeErr {
	bool operator()(const struct Edge& e1, const struct Edge& e2) const {
		if (e2.edgeError < e1.edgeError)
			return true;
		return false;
	}
};


class OBJModel{
public:
    std::list<OBJIndex> OBJIndices;
	std::vector<glm::mat4> errors;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
	std::vector<glm::vec3> colors;
	void OBJModel::printFaces(std::list<OBJIndex> faces);
	void setNeighbor();
	void printVector(std::vector<Edge> vec);
	bool hasUVs;
	bool findEdge(struct Edge &e);
    bool hasNormals;
    

	std::vector<Edge> edgeVector;																		//@Added
	std::multimap<int, int> vertexNeighbor;																//@Added
	std::vector<Edge> OBJModel::removeDups(std::vector<Edge> toRemove);									//@Added
	void calcEdge(std::list<OBJIndex> faces);															//@Added
	void buildHeap();																					//@Added
	void start(int maxFaces);																			//@Added
	void OBJModel::calcFaces(std::list<OBJIndex> faces, int firstEdgeInd, int secondEdgeInd);			//@Added
	glm::vec4 OBJModel::getOptimalPos(glm::mat4 m, glm::vec4 Y);										//@Added
	glm::vec4 OBJModel::calcOptimalPos(Edge& e, glm::mat4 m);											//@Added
	void OBJModel::calcEdgeError2(struct Edge& e);														//@Added
	glm::mat4 OBJModel::calcVertexError(int vertexIndex);												//@Added
	OBJModel(const std::string& fileName, int simplifyFlag);											//Edited
	void OBJModel::calcEdgeError(struct Edge &e);	
	bool OBJModel::isTriangle(int second, int third);
    IndexedModel ToIndexedModel();


private:
    unsigned int FindLastVertexIndex(const std::vector<OBJIndex*>& indexLookup, const OBJIndex* currentIndex, const IndexedModel& result);
    void CreateOBJFace(const std::string& line);
    glm::vec2 ParseOBJVec2(const std::string& line);
    glm::vec3 ParseOBJVec3(const std::string& line);
    OBJIndex ParseOBJIndex(const std::string& token, bool* hasUVs, bool* hasNormals);
	void CalcNormals();

};

#endif // OBJ_LOADER_H_INCLUDED
