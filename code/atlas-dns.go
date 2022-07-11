/* Do not ask what this script does, i also don't know it :) */
package main

import (
	"bufio"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"net/http"
	"os"
	"strconv"
	"strings"
	"time"
)

const (
	URL  string = "https://atlas.ripe.net/api/v1/measurement/"
	DATA string = "{ \"definitions\": [ { \"target\": \"d.nic.fr\", \"query_argument\": \"fr\", \"query_class\": \"IN\", \"query_type\": \"SOA\", \"description\": \"DNS AFNIC\", \"type\": \"dns\", \"af\": 6, \"is_oneoff\": \"True\"} ],  \"probes\": [  { \"requested\": 5, \"type\": \"area\", \"value\": \"WW\" } ] }"
)

func main() {
	var (
		object      interface{}
		err         error
		mapObject   map[string]interface{}
		arrayObject []interface{}
	)
	measure := int64(0)
	flag.Parse()
	if flag.NArg() >= 2 {
		panic("Usage: dns [measurement_id]")
	} else if flag.NArg() == 1 {
		measure, err = strconv.ParseInt(flag.Arg(0), 10, 64)
		if err != nil {
			panic(err)
		}
	} else {
		// Pass
	}

	// Auth. info
	authfile := os.Getenv("HOME") + "/.atlas/auth"
	file, status := os.Open(authfile)
	if status != nil {
		panic(fmt.Sprintf("Cannot open \"%s\": %s\n", authfile, status))
	}
	rd := bufio.NewReader(file)
	line, status := rd.ReadString('\n')
	if status == io.EOF {
		panic("Empty file")
	}
	key := "?key=" + line[0:len(line)-1]

	// Preparation
	client := &http.Client{}
	if measure == 0 {
		data := strings.NewReader(DATA)
		req, err := http.NewRequest("POST", URL+key, data)
		if err != nil {
			panic(err)
		}
		req.Header.Add("Content-Type", "application/json")
		req.Header.Add("Accept", "application/json")
		fmt.Printf("%s\n\n", DATA)

		// Request
		response, err := client.Do(req)
		if err != nil {
			panic(err)
		}
		body, err := ioutil.ReadAll(response.Body)
		if err != nil {
			panic(err)
		}
		// fmt.Printf("%s\n", body)
		err = json.Unmarshal(body, &object)
		if err != nil {
			panic(err)
		}
		response.Body.Close()
		mapObject = object.(map[string]interface{})
		// Yes, the json Go package treats integers in JSON as float64 :-(
		measure = int64(mapObject["measurements"].([]interface{})[0].(float64))
		fmt.Printf("Measurement #%v\n", measure)
	}

	// Retrieve results
	over := false
	req, err := http.NewRequest("GET", fmt.Sprintf("%s%v/%s", URL, measure, key), nil)
	if err != nil {
		panic(err)
	}
	req.Header.Add("Accept", "application/json")
	for !over {
		response, err := client.Do(req)
		if err != nil {
			panic(fmt.Sprintf("Cannot retrieve measurement status: %s", err))
		}
		body, err := ioutil.ReadAll(response.Body)
		if err != nil {
			panic(err)
		}
		err = json.Unmarshal(body, &object)
		if err != nil {
			panic(err)
		}
		response.Body.Close()
		mapObject = object.(map[string]interface{})
		status := mapObject["status"].(map[string]interface{})["name"].(string)
		if status == "Ongoing" || status == "Specified" {
			fmt.Printf("Not yet ready, be patient...\n")
			time.Sleep(60 * time.Second)
		} else if status == "Stopped" {
			over = true
		} else {
			fmt.Printf("Unknown status %s\n", status)
			time.Sleep(90 * time.Second)
		}
	}
	req, err = http.NewRequest("GET", fmt.Sprintf("%s%v/result/%s", URL, measure, key), nil)
	if err != nil {
		panic(err)
	}
	req.Header.Add("Accept", "application/json")
	response, err := client.Do(req)
	if err != nil {
		panic(fmt.Sprintf("Cannot retrieve measurement results: %s", err))
	}
	body, err := ioutil.ReadAll(response.Body)
	if err != nil {
		panic(err)
	}
	err = json.Unmarshal(body, &object)
	if err != nil {
		panic(err)
	}
	response.Body.Close()
	arrayObject = object.([]interface{})
	total_rtt := float64(0)
	num_rtt := 0
	num_error := 0
	for i := range arrayObject {
		mapObject := arrayObject[i].(map[string]interface{})
		result, present := mapObject["result"]
		if present {
			rtt := result.(map[string]interface{})["rt"].(float64)
			num_rtt++
			total_rtt += rtt
		} else {
			num_error++
		}
	}
	fmt.Printf("%v successes, %v failures, average RTT %v\n", num_rtt, num_error, total_rtt/float64(num_rtt))
}
