/* Demo program to parse CBOR (RFC 7049) files and display them as a
tree. */

package main

import (
	"bufio"
	"encoding/binary"
	"flag"
	"fmt"
	"io"
	"os"
)

var (
	filename string
	verbose  int
	depth    uint
)

func parseAdditionalInteger(r *bufio.Reader, initial byte) int64 {
	var (
		buf  []byte
		buf2 []byte
		buf4 []byte
	)
	val := uint(initial)
	buf = make([]byte, 1)
	buf2 = make([]byte, 2)
	buf4 = make([]byte, 4)
	switch {
	case val <= 23:
		return int64(val)
	case val == 24:
		n, err := r.Read(buf)
		if err != nil || n != 1 {
			fmt.Fprintf(os.Stderr, "Error reading additional integer in \"%s\": %s\n", filename, err)
			os.Exit(1)
		}
		return int64(buf[0])
	case val == 25:
		n, err := r.Read(buf2)
		if err != nil || n != 2 {
			fmt.Fprintf(os.Stderr, "Error reading additional integer in \"%s\": %s\n", filename, err)
			os.Exit(1)
		}
		return int64(binary.BigEndian.Uint16(buf2))
	case val == 26:
		n, err := r.Read(buf4)
		if err != nil || n != 4 {
			fmt.Fprintf(os.Stderr, "Error reading additional integer in \"%s\": %s\n", filename, err)
			os.Exit(1)
		}
		return int64(binary.BigEndian.Uint32(buf4))
	case val == 31:
		return -1
	default:
		fmt.Fprintf(os.Stderr, "Unsupported case in parseAdditionalInteger: %d\n", initial)
		os.Exit(1)
	}
	// Never reached
	return int64(initial)
}

func readCbor(r *bufio.Reader, eofPossible bool) bool {
	var (
		buffer []byte
		i      uint
	)
	buffer = make([]byte, 1)
	n, err := r.Read(buffer)
	if err != nil {
		if err == io.EOF && eofPossible {
			// Normal end of file
			os.Exit(0)
		} else {
			fmt.Fprintf(os.Stderr, "Error reading first byte of \"%s\": %s\n", filename, err)
			os.Exit(1)
		}
	}
	if n != 1 {
		fmt.Fprintf(os.Stderr, "Error reading first byte of \"%s\": expected %d, got %d bytes\n", filename, 1, n)
		os.Exit(1)
	}
	majorType := uint(buffer[0] >> 5) // First 3 bits
	add := buffer[0] & 0x001F         // Last 5 bits
	if verbose >= 1 {
		for i = 0; i < depth; i++ {
			fmt.Fprintf(os.Stdout, "\t")
		}
	}
	switch {
	case majorType == 0:
		additional := parseAdditionalInteger(r, add)
		if verbose >= 1 {
			fmt.Fprintf(os.Stdout, "Unsigned integer %d\n", additional)
		}
	case majorType == 2:
		additional := parseAdditionalInteger(r, add)
		if verbose >= 1 {
			fmt.Fprintf(os.Stdout, "Byte string of length %d\n", additional)
		}
		buffer = make([]byte, additional)
		n, err := r.Read(buffer)
		if err != nil {
			fmt.Fprintf(os.Stdout, "Error reading byte string \"%s\": %s\n", filename, err)
			os.Exit(1)
		}
		if n != int(additional) {
			fmt.Fprintf(os.Stdout, "Short read of byte string \"%s\": expected %d, got %d\n", filename, additional, n)
			os.Exit(1)
		}
	// Binary, don't display it
	case majorType == 3:
		additional := parseAdditionalInteger(r, add)
		if verbose >= 1 {
			fmt.Fprintf(os.Stdout, "String of length %d: ", additional)
		}
		buffer = make([]byte, additional)
		n, err := r.Read(buffer)
		if err != nil || n != int(additional) {
			fmt.Fprintf(os.Stdout, "Error reading string \"%s\": %s\n", filename, err)
			os.Exit(1)
		}
		if verbose >= 1 {
			fmt.Fprintf(os.Stdout, "%s\n", buffer[0:n])
		}
	case majorType == 4:
		additional := parseAdditionalInteger(r, add)
		if additional != -1 {
			if verbose >= 1 {
				fmt.Fprintf(os.Stdout, "Array of %d items\n", additional)
			}
			depth += 1
			for i = 0; i < uint(additional); i++ {
				readCbor(r, false)
			}
			depth -= 1
		} else {
			if verbose >= 1 {
				fmt.Fprintf(os.Stdout, "Array of indefinite number of items\n")
			}
			depth += 1
			over := false
			for !over {
				over = readCbor(r, false)
			}
		}
	case majorType == 5:
		additional := parseAdditionalInteger(r, add)
		if additional != -1 {
			if verbose >= 1 {
				fmt.Fprintf(os.Stdout, "Map of %d items\n", additional)
			}
			depth += 1
			for i = 0; i < uint(additional); i++ {
				// Key
				readCbor(r, false)
				// Value
				fmt.Fprintf(os.Stdout, " => ")
				readCbor(r, false)
			}
			depth -= 1
		} else {
			if verbose >= 1 {
				fmt.Fprintf(os.Stdout, "Map of indefinite number of items\n")
			}
			depth += 1
			over := false
			for !over {
				// Key
				over = readCbor(r, false)
				// Value
				if !over {
					fmt.Fprintf(os.Stdout, " => ")
					readCbor(r, false)
				}
			}
		}
	case majorType == 6:		
		additional := parseAdditionalInteger(r, add)                
		if verbose >= 1 {
			fmt.Fprintf(os.Stdout, "Tag %d\n", additional)
		}
		depth += 1
		readCbor(r, false)
		depth -= 1
	case majorType == 7:
		depth -= 1
		if verbose >= 1 {
			fmt.Fprintf(os.Stdout, "End of array/map\n")
		}
		return true
	default:
		fmt.Fprintf(os.Stdout, "Unsupported case in readCbor: %d\n", majorType)
		os.Exit(1)
	}
	return false
}

func main() {
	var (
		file *os.File
		err error
	)
	verbose = 1
	flag.Parse()
	if flag.NArg() == 1 {
		filename = flag.Arg(0)
	} else {
		fmt.Fprintf(os.Stderr, "Usage: read-cbor filename\n")
		os.Exit(1)
	}
	if filename != "-" {		
		file, err = os.Open(filename)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Cannot open \"%s\": %s\n", filename, err)
			os.Exit(1)
		}
	} else {
		file = os.Stdin
	}
	rd := bufio.NewReader(file)
	over := false
	depth = 0
	for !over {
		readCbor(rd, true)
	}
}
