import utils
from server import Server


# main function, starts by reading the port, then sets up and starts the server object.
def main():
    port = utils.get_port()
    server = Server("", port)
    server.start()


if __name__ == "__main__":
    main()
