## cc-oci-runtime integration tests

### Docker

1. Enable the Docker integration tests:

```
./autogen.sh --enable-docker-tests
```

2. Run the Docker integration tests:

```
sudo -E make docker-tests
```

### CRI-O

1. Enable the CRI-O integration tests:

```
./autogen.sh --enable-crio-tests
```

2. Run the CRI-O integration tests:

```
sudo -E make crio-tests
```
