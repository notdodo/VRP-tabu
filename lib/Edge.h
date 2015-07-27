#ifndef Edge_H
#define Edge_H

class Edge {
public:
    // Only class Vertex can create an object Edge
    class ConstructionToken {
    private:
        ConstructionToken();
        friend class Vertex;
    };

    Edge(const Edge &);
    Edge(const ConstructionToken &, const int &);
    int weight;
private:
};

#endif /* Edge_H */