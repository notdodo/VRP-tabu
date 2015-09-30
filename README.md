# VRP
This repo contains the project of "Metodi ed Algoritmi di Ottimizzazione per il Problem Solving": an algorithm for [VRP](https://en.wikipedia.org/wiki/Vehicle_routing_problem) with [tabu search](https://en.wikipedia.org/wiki/Tabu_search) heuristic.
This program reads data from a JSON file which holds all informations about customers, vehicles, depot and requests.

This simple example shows a configuration of 3 customers to serve with 2 vehicles with max capacity of 400 and max work time of 350. Each customer is has a name, a position, an amount of resources requested and a time to get served. In order to create the best routes it's necessary to know the cost from each travel from each customers (in this case the distance is the geometric distance).
```json
{
  "vertices": [
    {
      "name": "v0",
      "x": 0,
      "y": 0
    },
    {
      "name": "v1",
      "x": -17,
      "y": 181,
      "request": 33,
      "time": 16
    },
    {
      "name": "v2",
      "x": -237,
      "y": -13,
      "request": 7,
      "time": 11
    },
    {
      "name": "v3",
      "x": -170,
      "y": -193,
      "request": 15,
      "time": 9
    }
  ],
  "vehicles": 2,
  "capacity": 400,
  "worktime": 350,
  "costs": [
    [
      {
        "from": 0,
        "to": 1,
        "value": 181
      },
      {
        "from": 0,
        "to": 2,
        "value": 237
      },
      {
        "from": 0,
        "to": 3,
        "value": 257
      }
    ],
    [
      {
        "from": 1,
        "to": 0,
        "value": 181
      },
      {
        "from": 1,
        "to": 2,
        "value": 293
      },
      {
        "from": 1,
        "to": 3,
        "value": 404
      }
    ],
    [
      {
        "from": 2,
        "to": 0,
        "value": 237
      },
      {
        "from": 2,
        "to": 1,
        "value": 293
      },
      {
        "from": 2,
        "to": 3,
        "value": 192
      }
    ],
    [
      {
        "from": 3,
        "to": 0,
        "value": 257
      },
      {
        "from": 3,
        "to": 1,
        "value": 404
      },
      {
        "from": 3,
        "to": 2,
        "value": 192
      }
    ]
  ]
}
```
The position of each customer matches a coordinate in the Cartesian Plane (with 4 quadrants).

After the execution of the program the output file should be like this:
```json
{
    "vertices": [
        {
            "name": "v0",
            "x": 0,
            "y": 0
        },
        {
            "name": "v1",
            "x": -17,
            "y": 181,
            "request": 33,
            "time": 16
        },
        {
            "name": "v2",
            "x": -237,
            "y": -13,
            "request": 7,
            "time": 11
        },
    [....]
            {
                "from": 4,
                "to": 2,
                "value": 408
            },
            {
                "from": 4,
                "to": 3,
                "value": 427
            }
        ]
    ],
    "routes": [
        [
            "v0",
            "v1",
            "v2",
            "v3",
            "v0"
        ],
        [
            "v0",
            "v4",
            "v0"
        ]
    ],
    "costs": [
        923,
        356
    ]
}
```
The program appended two more attribute which represent the final routes with relative costs.

## Web-UI
Create files as the example above is something long and tedious, expecially for complex initial configurations.
The Web-UI, created in HTML5, CSS3 and JavaScript, allows easily creation of input files in JSON simply creating the customers in a click. User can set all other settings without writing a single line of JSON.

![Example with customers](https://raw.githubusercontent.com/edoz90/VRP-tabu/master/screenshot/customers.png "Example")

The application could be run in a web server or simply opening the `coords.html` file.

After creating and executing the JSON file through the VRP program the Web-UI can read the output file to display costs and routes.

![Example of routes](https://raw.githubusercontent.com/edoz90/VRP-tabu/master/screenshot/routes.png "Routes and costs")