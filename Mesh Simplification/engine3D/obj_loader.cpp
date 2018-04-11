#include "obj_loader.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <set>

static bool CompareOBJIndexPtr(const OBJIndex* a, const OBJIndex* b);
static inline unsigned int FindNextChar(unsigned int start, const char* str, unsigned int length, char token);
static inline unsigned int ParseOBJIndexValue(const std::string& token, unsigned int start, unsigned int end);
static inline float ParseOBJFloatValue(const std::string& token, unsigned int start, unsigned int end);
static inline std::vector<std::string> SplitString(const std::string &s, char delim);


// ----------------------------------------------------------- //
// Change this value to the wanted simplification ------------ //
// NOTE: Pay attention to the object the main loads----------- //
// MAX_FACES = the maximum number of faces the  -------------- //
// simplyfied OBJ will have. --------------------------------- //
// ----------------------------------------------------------- //
#define MAX_FACES 3500


// OBJModel ctor
OBJModel::OBJModel(const std::string& fileName, int simplifyFlag){

	hasUVs = false;
	hasNormals = false;
    std::ifstream file;
    file.open(fileName.c_str());
	int bla1;
    std::string line;

    if(file.is_open()){
        while(file.good()){
            getline(file, line);
            unsigned int lineLength = line.length();
            if(lineLength < 2)
                continue;

            const char* lineCStr = line.c_str();
            
            switch(lineCStr[0])            {
                case 'v':
                    if(lineCStr[1] == 't')
                        this->uvs.push_back(ParseOBJVec2(line));
                    else if(lineCStr[1] == 'n'){
                        this->normals.push_back(ParseOBJVec3(line));
						this->colors.push_back((normals.back()+glm::vec3(1,1,1))*0.5f);
					}
					else if(lineCStr[1] == ' ' || lineCStr[1] == '\t')
                        this->vertices.push_back(ParseOBJVec3(line));
                break;
                case 'f':
                    CreateOBJFace(line);
                break;
                default: break;
            };
        }

		calcEdge(OBJIndices);	//@Added init edgeVector.
		setNeighbor();			//@Added init neighbors multimap
		for (int i = 0; i < vertices.size(); i++){	
			errors.push_back(calcVertexError(i));
		}
		for (int i = 0; i < edgeVector.size(); i++) {
			calcEdgeError(edgeVector[i]);
		}

		buildHeap();

		//using the simplifyFlag to create simplified Mesh
		if(simplifyFlag == 1)
			start(MAX_FACES);
	}
    else
        std::cerr << "Unable to load mesh: " << fileName << std::endl;
}

// printFaces takes a list of OBJIndex and prints
// all the current faces the mesh has
void OBJModel::printFaces(std::list<OBJIndex> faces) {
	int counter = 0;
	OBJIndex first;
	OBJIndex second;
	OBJIndex third;
	for (std::list<OBJIndex>::iterator it = faces.begin(); it != faces.end(); it++) {
		if (counter == 0) 
			first = *it;
		if (counter == 1) 
			second = *it;
		if (counter == 2) {
			third = *it;
			counter = 0;
			std::cout << "first: " << first.vertexIndex << " second: " << second.vertexIndex << " third: " << third.vertexIndex << std::endl;
			continue;
		}
		counter++;
	}

}

//@Added 
// given the list of faces and 2 vertex indices
// removes the 2 faces including both indices
// removes the edges the will become duplicates by the removal of the edge of these indices
void OBJModel::calcFaces(std::list<OBJIndex> faces,int firstEdgeInd, int secondEdgeInd) {
	int counter = 0;
	OBJIndex first;
	OBJIndex second;
	OBJIndex third;
	bool goback = false;
	std::list<OBJIndex>::iterator it2 = OBJIndices.begin();
	for (std::list<OBJIndex>::iterator it = OBJIndices.begin(); it != OBJIndices.end();it++) {
		if (goback) {
			it--;
			goback = false;
		}
		if (counter == 0) {
			first = *it;
			it2 = it;
		}
		if (counter == 1) {
			second = *it;
		}
		if (counter == 2) {
			third = *it;
			counter = 0;


			if ((firstEdgeInd == first.vertexIndex || firstEdgeInd == second.vertexIndex || firstEdgeInd == third.vertexIndex) && (secondEdgeInd == first.vertexIndex || secondEdgeInd == second.vertexIndex || secondEdgeInd == third.vertexIndex)) {
				for (int i = 0 ; i < edgeVector.size() ; ) {
					if ((edgeVector[i].firstVertex.vertexIndex == first.vertexIndex || edgeVector[i].firstVertex.vertexIndex == second.vertexIndex || edgeVector[i].firstVertex.vertexIndex == third.vertexIndex) && (edgeVector[i].secondVertex.vertexIndex == first.vertexIndex || edgeVector[i].secondVertex.vertexIndex == second.vertexIndex || edgeVector[i].secondVertex.vertexIndex == third.vertexIndex)) {
						if (edgeVector[i].firstVertex.vertexIndex != firstEdgeInd && edgeVector[i].secondVertex.vertexIndex != firstEdgeInd) {
							edgeVector.erase(edgeVector.begin() + i);
						}
						else if(edgeVector[i].firstVertex.vertexIndex == firstEdgeInd && edgeVector[i].secondVertex.vertexIndex == firstEdgeInd)
							edgeVector.erase(edgeVector.begin() + i);
						else
							i++;
					}
					else
						i++;
				}
				it--;
				it--;
				OBJIndices.erase(it++);
				OBJIndices.erase(it++);
				OBJIndices.erase(it++);

				if (it != OBJIndices.begin()) 
					it--;
				else
					goback = true;
			}
			continue;
		}
			counter++;	
	}
	
	for (std::list<OBJIndex>::iterator it3 = OBJIndices.begin(); it3 != OBJIndices.end();it3++) {
		if ((*it3).vertexIndex == secondEdgeInd)
			(*it3).vertexIndex = firstEdgeInd;
	}
}



//@Added 
// calcEdges initialize EdgeVector and removes
// duplicates Edges
void OBJModel::calcEdge(std::list<OBJIndex> faces) {
	int counter = 0;
	OBJIndex first;
	OBJIndex second;
	OBJIndex third;
	for (std::list<OBJIndex>::iterator it = faces.begin(); it !=faces.end(); it++) {

		if (counter == 0) 
			first = *it;			
		if (counter == 1) 
			second = *it;
		if (counter == 2) {
			third = *it;
			counter = 0;
			struct Edge edge1;
			struct Edge edge2;
			struct Edge edge3;

			edge1.firstVertex = first;
			edge1.secondVertex= second;

			if (first.vertexIndex > second.vertexIndex) {
				edge1.firstVertex = second;
				edge1.secondVertex = first;
			}

			edge2.firstVertex = first;
			edge2.secondVertex = third;

			if (first.vertexIndex > third.vertexIndex) {
				edge1.firstVertex = third;
				edge1.secondVertex = first;
			}
			edge3.firstVertex = second;
			edge3.secondVertex = third;

			if (second.vertexIndex > third.vertexIndex) {
				edge1.firstVertex = third;
				edge1.secondVertex = second;
			}
			edgeVector.push_back(edge1);
			edgeVector.push_back(edge2);
			edgeVector.push_back(edge3);
			continue;
		}
		counter++;
	}
	edgeVector = removeDups(edgeVector);
 }


//@Added 
// setNeighbor initialize vertexNeighbor multimap
void  OBJModel::setNeighbor() {
	for (size_t i = 0; i < vertices.size(); i++){
		for (size_t j = 0; j < edgeVector.size(); j++){
			if (i == edgeVector[j].firstVertex.vertexIndex)
				vertexNeighbor.insert(std::pair<int, int>(i, edgeVector[j].secondVertex.vertexIndex));
			if (i == edgeVector[j].secondVertex.vertexIndex)
				vertexNeighbor.insert(std::pair<int, int>(i, edgeVector[j].firstVertex.vertexIndex));
		}
	}
}

//@Added 
// calculates the Quadric Error of a vertex
glm::mat4 OBJModel::calcVertexError(int vertexIndex) {
	glm::mat4 qMat;
	std::pair <std::multimap<int, int>::iterator, std::multimap<int, int>::iterator> result = vertexNeighbor.equal_range(vertexIndex);
	for (std::multimap<int, int>::iterator it = result.first; it != result.second; it++)
	{

		for (std::multimap<int, int>::iterator it2 = it; it2 != result.second; it2++)
		{
			if (it2->second != it->second) {
				// up untill here we check if the 3 indexes creates a triangle
				if (isTriangle(it->second, it2->second)) {
				
					//calc cross prod
					glm::vec3 n = glm::cross(vertices[it2->second] - vertices[vertexIndex], vertices[it->second] - vertices[vertexIndex]);
					n = glm::normalize(n);
				
					glm::vec4 v_tag = glm::vec4(n, -(glm::dot(vertices[vertexIndex], n)));
					for (int i = 0;i < 4;i++) {
						for (int j = 0;j < 4;j++) {
							qMat[i][j] += v_tag[i] * v_tag[j];
						}
					}
				}
			}
		}
	}
	return qMat;
}


void OBJModel::buildHeap() {
	std::make_heap(edgeVector.begin(), edgeVector.end(), compEdgeErr());
}

void OBJModel::calcEdgeError(struct Edge& e) {
	e.edgeQ = errors[e.firstVertex.vertexIndex] + errors[e.secondVertex.vertexIndex];
	
	// calc new position in the middle
	glm::vec4 midVec = glm::vec4((vertices[e.firstVertex.vertexIndex] + vertices[e.secondVertex.vertexIndex]) / 2.0f , 1.0);
	e.newPos = glm::vec3(midVec);
	
	glm::vec4 temp = e.edgeQ * midVec; // temp is a row vector
	e.edgeError = glm::dot(midVec, temp);
}

// calculates the cost of contracting the edge e,
// based on the optimal position of the new vertex
void OBJModel::calcEdgeError2(struct Edge& e) {
	
	e.edgeQ = errors[e.firstVertex.vertexIndex] + errors[e.secondVertex.vertexIndex];
	glm::vec4 v_tag = calcOptimalPos(e, e.edgeQ);

	glm::vec4 temp = e.edgeQ * v_tag; // temp is a row vector
	e.edgeError = glm::dot(v_tag, temp);
}


bool OBJModel::isTriangle(int second, int third) {
	//second & third already in first
	std::pair <std::multimap<int, int>::iterator, std::multimap<int, int>::iterator> result = vertexNeighbor.equal_range(second);
	for (std::multimap<int, int>::iterator it = result.first; it != result.second; it++) {
		if (third == it->second) {
			return true;
		}
	}
	return false;
}


bool OBJModel::findEdge(struct Edge &e) {
	int counter = 0;
	for (int i = 0; i < edgeVector.size(); i++) {
		if (e == edgeVector[i] && counter == 0)
			counter++;
		else if (e == edgeVector[i])
			return true;
	}
	return false;
}

//@Added
// helper function to remove duplicated edges from EdgeVector
std::vector<Edge> OBJModel::removeDups(std::vector<Edge> toRemove){ 
	unsigned int i;
	unsigned int j;
	std::vector<Edge> noDup;
	bool found = false;
	for (i = 0; i < toRemove.size(); i++) {
		found = false;
		for (j = i+1; j < toRemove.size() && !found; j++) {			
			if (toRemove[i] == toRemove[j])
				found = true;
		}
		if (!found)
			noDup.push_back(toRemove[i]);
	}
	
	return noDup;
}

// Prints vector of Edges
void OBJModel::printVector(std::vector<Edge> vec){
	std::cout << "Start Print and edge size: "<< edgeVector.size() << std::endl;
	for (unsigned int i = 0; i < vec.size(); i++){
		std::cout << "edge["<<i<<"] = " << vec[i].firstVertex.vertexIndex <<"  "<< vec[i].secondVertex.vertexIndex  << " Error: " <<vec[i].edgeError<< std::endl;
	}
}


void OBJModel::CalcNormals(){
	float *count = new float[normals.size()];
	for (int i = 0; i < normals.size(); i++){
		count[i] = 0;
	}
	for (std::list<OBJIndex>::iterator it = OBJIndices.begin(); it !=OBJIndices.end(); it++){
		int i0 = (*it).vertexIndex;
		(*it).normalIndex = i0;
		it++;
		int i1 = (*it).vertexIndex;
		(*it).normalIndex = i1;
		it++;
		int i2 = (*it).vertexIndex;
		(*it).normalIndex = i2;
		glm::vec3 v1 = vertices[i1] - vertices[i0];
		glm::vec3 v2 = vertices[i2] - vertices[i0];
		glm::vec3 normal = glm::normalize(glm::cross(v2, v1));

		if (count[i0] == 0)
			count[i0] = 1.0f;
		else
			count[i0] = count[i0] / (count[i0] + 1);

		if (count[i1] == 0)
			count[i1] = 1.0f;
		else
			count[i1] = count[i1] / (count[i1] + 1);

		if (count[i2] == 0)
			count[i2] = 1.0f;
		else
			count[i2] = count[i2] / (count[i2] + 1);
		
		normals[i0] += normal;
		normals[i1] += normal;
		normals[i2] += normal;
	}
	for (int i = 0; i < normals.size(); i++){
		normals[i] = normals[i] * count[i];
	}
	delete count;
}
 
IndexedModel OBJModel::ToIndexedModel(){
    IndexedModel result;
    IndexedModel normalModel;
	IndexedModel simpleResult;
    
    unsigned int numIndices = OBJIndices.size();
    std::vector<OBJIndex*> indexLookup;
 
	 if(!hasNormals){
		 for (int i = 0; i < vertices.size(); i++){
			normals.push_back(glm::vec3(0,0,0));
		 }
		 hasNormals = true;
	 }
	 CalcNormals();

 	 for(OBJIndex &it1 : OBJIndices){
        indexLookup.push_back(&it1);
	}
    
	 std::sort(indexLookup.begin(), indexLookup.end(), CompareOBJIndexPtr);
    std::map<OBJIndex, unsigned int> normalModelIndexMap;
    std::map<unsigned int, unsigned int> indexMap;

    for(OBJIndex &it1 : OBJIndices){
        OBJIndex* currentIndex = &it1;
  
        glm::vec3 currentPosition = vertices[currentIndex->vertexIndex];
        glm::vec2 currentTexCoord;
        glm::vec3 currentNormal;
        glm::vec3 currentColor;

        if(hasUVs)
            currentTexCoord = uvs[currentIndex->uvIndex];
        else
            currentTexCoord = glm::vec2(0,0);
            
		currentNormal = normals[currentIndex->normalIndex];
		currentColor = normals[currentIndex->normalIndex];
        
		unsigned int normalModelIndex;
        unsigned int resultModelIndex;
        
        //Create model to properly generate normals on
        std::map<OBJIndex, unsigned int>::iterator it = normalModelIndexMap.find(*currentIndex);
        if(it == normalModelIndexMap.end()){
            normalModelIndex = normalModel.positions.size();
            normalModelIndexMap.insert(std::pair<OBJIndex, unsigned int>(*currentIndex, normalModelIndex));
            normalModel.positions.push_back(currentPosition);
            normalModel.texCoords.push_back(currentTexCoord);
            normalModel.normals.push_back(currentNormal);
			normalModel.colors.push_back(currentColor);
        }
        else
            normalModelIndex = it->second;
        
        //Create model which properly separates texture coordinates
        unsigned int previousVertexLocation = FindLastVertexIndex(indexLookup, currentIndex, result);
        
        if(previousVertexLocation == (unsigned int)-1){
            resultModelIndex = result.positions.size();
            result.positions.push_back(currentPosition);
            result.texCoords.push_back(currentTexCoord);
            result.normals.push_back(currentNormal);
			result.colors.push_back(currentColor);
        }
        else
            resultModelIndex = previousVertexLocation;
        
        normalModel.indices.push_back(normalModelIndex);
        result.indices.push_back(resultModelIndex);
        indexMap.insert(std::pair<unsigned int, unsigned int>(resultModelIndex, normalModelIndex));
    }
    return result;
};

// in this section is the simplification algorithm

void OBJModel::start(int maxFaces) {
	int currFaces = OBJIndices.size()/3 ;
	while (currFaces > maxFaces) {
		std::pop_heap(edgeVector.begin(), edgeVector.end(), compEdgeErr());

		Edge removedEdge = edgeVector.back();

		int firstVertexInd = removedEdge.firstVertex.vertexIndex;
		int secondVertexInd = removedEdge.secondVertex.vertexIndex;

		glm::vec3 newVertex = removedEdge.newPos;
		errors[firstVertexInd] = removedEdge.edgeQ;

		vertices[firstVertexInd] = newVertex;
	
		// for loop over all the vertices
		for (int ch = 0; ch < vertices.size(); ch++){
			int count = 0;
			std::pair <std::multimap<int, int>::iterator, std::multimap<int, int>::iterator> ret;
			ret = vertexNeighbor.equal_range(ch);
				for (std::multimap<int, int>::iterator it = ret.first; it != ret.second;) {
						if (ch != firstVertexInd) {
							if (it->second == secondVertexInd && count == 0) {
								it->second = firstVertexInd;
								count++;
								it++;

							}
							else if (it->second == secondVertexInd) {
								vertexNeighbor.erase(it++);
							}
							else
								it++;
						}
						else {
							if (it->second == secondVertexInd)
								vertexNeighbor.erase(it++);
							else
								it++;
						}
				}
		}
		for (int ch = 0; ch < vertices.size(); ch++){
			int count = 0;

			std::pair <std::multimap<int, int>::iterator, std::multimap<int, int>::iterator> ret;
			ret = vertexNeighbor.equal_range(ch);
			for (std::multimap<int, int>::iterator it = ret.first; it != ret.second;) {
				if (it->second == firstVertexInd && count == 0) {
					count++;
					it++;
				}
				else if (it->second == firstVertexInd)
						vertexNeighbor.erase(it++);
				else
					it++;
			}
		}

		// removes the edge with minimal error
		edgeVector.pop_back();
		
		// reduce faces count
		currFaces = currFaces - 2;
		
		// removing the deleted vertex in the neighbors multimap
		vertexNeighbor.erase(secondVertexInd);
		
		// Calculate new faces
		calcFaces(OBJIndices, firstVertexInd, secondVertexInd);

		// updating edges new verticies
		for (int i = 0; i < edgeVector.size(); i++) {
			
			if (edgeVector[i].secondVertex.vertexIndex == secondVertexInd) 
				edgeVector[i].secondVertex.vertexIndex = firstVertexInd;
	
		   else if (edgeVector[i].firstVertex.vertexIndex == secondVertexInd) 
				edgeVector[i].firstVertex.vertexIndex = firstVertexInd;
		}
		for (int i = 0; i < edgeVector.size(); i++) {
			if (edgeVector[i].firstVertex.vertexIndex == firstVertexInd || edgeVector[i].secondVertex.vertexIndex == firstVertexInd) {
				calcEdgeError(edgeVector[i]);
			}
		}

		buildHeap();
	}
}


unsigned int OBJModel::FindLastVertexIndex(const std::vector<OBJIndex*>& indexLookup, const OBJIndex* currentIndex, const IndexedModel& result)
{
    unsigned int start = 0;
    unsigned int end = indexLookup.size();
    unsigned int current = (end - start) / 2 + start;
    unsigned int previous = start;
    
    while(current != previous)
    {
        OBJIndex* testIndex = indexLookup[current];
        
        if(testIndex->vertexIndex == currentIndex->vertexIndex)
        {
            unsigned int countStart = current;
        
            for(unsigned int i = 0; i < current; i++)
            {
                OBJIndex* possibleIndex = indexLookup[current - i];
                
                if(possibleIndex == currentIndex)
                    continue;
                    
                if(possibleIndex->vertexIndex != currentIndex->vertexIndex)
                    break;
                    
                countStart--;
            }
            
            for(unsigned int i = countStart; i < indexLookup.size() - countStart; i++)
            {
                OBJIndex* possibleIndex = indexLookup[current + i];
                
                if(possibleIndex == currentIndex)
                    continue;
                    
                if(possibleIndex->vertexIndex != currentIndex->vertexIndex)
                    break;
                else if((!hasUVs || possibleIndex->uvIndex == currentIndex->uvIndex) 
                    && (!hasNormals || possibleIndex->normalIndex == currentIndex->normalIndex))
                {
                    glm::vec3 currentPosition = vertices[currentIndex->vertexIndex];
                    glm::vec2 currentTexCoord;
                    glm::vec3 currentNormal;
                    glm::vec3 currentColor;

                    if(hasUVs)
                        currentTexCoord = uvs[currentIndex->uvIndex];
                    else
                        currentTexCoord = glm::vec2(0,0);
                        
                    if(hasNormals)
					{
                        currentNormal = normals[currentIndex->normalIndex];
						currentColor =  normals[currentIndex->normalIndex];
					}
					else
                    {
						currentNormal = glm::vec3(0,0,0);
						currentColor = glm::normalize(glm::vec3(1,1,1));
					}
                    for(unsigned int j = 0; j < result.positions.size(); j++)
                    {
                        if(currentPosition == result.positions[j] 
                            && ((!hasUVs || currentTexCoord == result.texCoords[j])
                            && (!hasNormals || currentNormal == result.normals[j])))
                        {
                            return j;
                        }
                    }
                }
            }
        
            return -1;
        }
        else
        {
            if(testIndex->vertexIndex < currentIndex->vertexIndex)
                start = current;
            else
                end = current;
        }
    
        previous = current;
        current = (end - start) / 2 + start;
    }
    
    return -1;
}

void OBJModel::CreateOBJFace(const std::string& line)
{
    std::vector<std::string> tokens = SplitString(line, ' ');
	unsigned int tmpIndex = OBJIndices.size();

	OBJIndices.push_back(ParseOBJIndex(tokens[1], &this->hasUVs, &this->hasNormals));

	OBJIndices.push_back(ParseOBJIndex(tokens[2], &this->hasUVs, &this->hasNormals));

	OBJIndices.push_back(ParseOBJIndex(tokens[3], &this->hasUVs, &this->hasNormals));

	if((int)tokens.size() > 4)//triangulation
    {	
		OBJIndices.push_back(ParseOBJIndex(tokens[1], &this->hasUVs, &this->hasNormals));
		
		OBJIndices.push_back(ParseOBJIndex(tokens[3], &this->hasUVs, &this->hasNormals));
		
		OBJIndices.push_back(ParseOBJIndex(tokens[4], &this->hasUVs, &this->hasNormals));
	
    }
}

OBJIndex OBJModel::ParseOBJIndex(const std::string& token, bool* hasUVs, bool* hasNormals)
{
    unsigned int tokenLength = token.length();
    const char* tokenString = token.c_str();
    
    unsigned int vertIndexStart = 0;
    unsigned int vertIndexEnd = FindNextChar(vertIndexStart, tokenString, tokenLength, '/');
    
    OBJIndex result;
    result.vertexIndex = ParseOBJIndexValue(token, vertIndexStart, vertIndexEnd);
    result.uvIndex = 0;
    result.normalIndex = 0;
	result.edgeIndex =-1;
	result.isEdgeUpdated = true;
    
    if(vertIndexEnd >= tokenLength)
        return result;
    
    vertIndexStart = vertIndexEnd + 1;
    vertIndexEnd = FindNextChar(vertIndexStart, tokenString, tokenLength, '/');
    
    result.uvIndex = ParseOBJIndexValue(token, vertIndexStart, vertIndexEnd);
    *hasUVs = true;
    
    if(vertIndexEnd >= tokenLength)
        return result;
    
    vertIndexStart = vertIndexEnd + 1;
    vertIndexEnd = FindNextChar(vertIndexStart, tokenString, tokenLength, '/');
    
    result.normalIndex = ParseOBJIndexValue(token, vertIndexStart, vertIndexEnd);
    *hasNormals = true;
    
    return result;
}

glm::vec3 OBJModel::ParseOBJVec3(const std::string& line) 
{
    unsigned int tokenLength = line.length();
    const char* tokenString = line.c_str();
    
    unsigned int vertIndexStart = 2;
    
    while(vertIndexStart < tokenLength)
    {
        if(tokenString[vertIndexStart] != ' ')
            break;
        vertIndexStart++;
    }
    
    unsigned int vertIndexEnd = FindNextChar(vertIndexStart, tokenString, tokenLength, ' ');
    
    float x = ParseOBJFloatValue(line, vertIndexStart, vertIndexEnd);
    
    vertIndexStart = vertIndexEnd + 1;
    vertIndexEnd = FindNextChar(vertIndexStart, tokenString, tokenLength, ' ');
    
    float y = ParseOBJFloatValue(line, vertIndexStart, vertIndexEnd);
    
    vertIndexStart = vertIndexEnd + 1;
    vertIndexEnd = FindNextChar(vertIndexStart, tokenString, tokenLength, ' ');
    
    float z = ParseOBJFloatValue(line, vertIndexStart, vertIndexEnd);
    
    return glm::vec3(x,y,z);
	
}

glm::vec2 OBJModel::ParseOBJVec2(const std::string& line)
{
    unsigned int tokenLength = line.length();
    const char* tokenString = line.c_str();
    
    unsigned int vertIndexStart = 3;
    
    while(vertIndexStart < tokenLength)
    {
        if(tokenString[vertIndexStart] != ' ')
            break;
        vertIndexStart++;
    }
    
    unsigned int vertIndexEnd = FindNextChar(vertIndexStart, tokenString, tokenLength, ' ');
    
    float x = ParseOBJFloatValue(line, vertIndexStart, vertIndexEnd);
    
    vertIndexStart = vertIndexEnd + 1;
    vertIndexEnd = FindNextChar(vertIndexStart, tokenString, tokenLength, ' ');
    
    float y = ParseOBJFloatValue(line, vertIndexStart, vertIndexEnd);
    
    return glm::vec2(x,y);
}

static bool CompareOBJIndexPtr(const OBJIndex* a, const OBJIndex* b)
{
    return a->vertexIndex < b->vertexIndex;
}

static inline unsigned int FindNextChar(unsigned int start, const char* str, unsigned int length, char token)
{
    unsigned int result = start;
    while(result < length)
    {
        result++;
        if(str[result] == token)
            break;
    }
    
    return result;
}

static inline unsigned int ParseOBJIndexValue(const std::string& token, unsigned int start, unsigned int end)
{
    return atoi(token.substr(start, end - start).c_str()) - 1;
}

static inline float ParseOBJFloatValue(const std::string& token, unsigned int start, unsigned int end)
{
    return atof(token.substr(start, end - start).c_str());
}

static inline std::vector<std::string> SplitString(const std::string &s, char delim)
{
    std::vector<std::string> elems;
        
    const char* cstr = s.c_str();
    unsigned int strLength = s.length();
    unsigned int start = 0;
    unsigned int end = 0;
        
    while(end <= strLength)
    {
        while(end <= strLength)
        {
            if(cstr[end] == delim)
                break;
            end++;
        }
            
        elems.push_back(s.substr(start, end - start));
        start = end + 1;
        end = start;
    }
        
    return elems;
}

glm::vec4 OBJModel::calcOptimalPos(Edge& e, glm::mat4 m) {
	glm::vec4 midVec = glm::vec4((vertices[e.firstVertex.vertexIndex] + vertices[e.secondVertex.vertexIndex]) / 2.0f, 1.0);
	m[3][0] = 0;
	m[3][1] = 0;
	m[3][2] = 0;
	m[3][3] = 1;

	glm::vec4 Y(0.0f, 0.0f, 0.0f, 1.0f);

	glm::vec4 ans = getOptimalPos(m, Y);
	if (ans[3] > 0.001)
		return glm::vec4(ans[0], ans[1], ans[2], 1.0f);
	else
		return midVec;
}


glm::vec4 OBJModel::getOptimalPos(glm::mat4 m, glm::vec4 Y) {
	for (int i = 0; i < 4; i++) {

		int j = 0;
		while (j < 4 && fabs(m[i][j]) < 0.001)
			j++;
		if (j == 4)
			continue;
		for (int k = 0; k < 4; k++) {
			if (k != i) {
				double rate = m[k][j] / m[i][j];
				for (int l = 0; l < 4; l++)
					m[k][l] -= m[i][l] * rate;
				Y[k] -= Y[i] * rate;
			}
		}
	
	}
	
	glm::vec4 X;
	for (int i = 0; i < 4; i++) {
		int j = 0;
		while (j < 4 && fabs(m[i][j]) < 0.001)
			j++;
		if (j == 4)
			return glm::vec4(0, 0, 0, -1);
		X[i] = Y[i] / m[i][j];
	}
	X[3] = 1;
	return X;
}