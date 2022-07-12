// Read the list of Paris subway stations
// https://data.ratp.fr/explore/dataset/positions-geographiques-des-stations-du-reseau-ratp/

package main

import "encoding/json"
import "os"
import "fmt"

// Must be large enough for the number of bytes in the JSON file
const SIZE = 100000000
const NUMBER = 10000

type StationFields struct {
	Fields Station
}

type Station struct {
	Stop_Id   int
	Stop_Name string
}

func main() {
	// Go is typed, you cannot convert any JSON object easily, you
	// have to know something about it (here, that is it an array
	// not, say, a map).
	allStations := make([]StationFields, NUMBER)
	data := make([]byte, SIZE)
	n, error := os.Stdin.Read(data)
	if error != nil {
		fmt.Printf("Cannot read from standard input: %s\n", error)
		os.Exit(1)
	}
	error = json.Unmarshal(data[0:n], &allStations)
	if error != nil {
		fmt.Printf("Cannot convert to JSON: %s\n", error)
		os.Exit(1)
	}
	fmt.Printf("%d stations\n", len(allStations))
	for i := 0; i < len(allStations); i++ {
		fmt.Printf("%s\n", allStations[i].Fields.Stop_Name)
	}
}
