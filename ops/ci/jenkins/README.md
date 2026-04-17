# Jenkins CI setup for chronos

## 1) Start Jenkins

```bash
cd /home/techmedaddy/projects/chronos/ops/ci/jenkins
docker compose -f setup-jenkins-docker-compose.yml up -d
```

Jenkins UI:
- http://localhost:8081

Get initial admin password:

```bash
docker exec -it chronos-jenkins cat /var/jenkins_home/secrets/initialAdminPassword
```

## 2) Install plugins

Install suggested plugins plus:
- Pipeline
- Git
- GitHub Integration
- Docker Pipeline
- Workspace Cleanup
- Timestamper
- AnsiColor

## 3) Create pipeline job

- New Item -> Pipeline
- Definition: Pipeline script from SCM
- SCM: Git
- Repo: your chronos repository URL
- Branch: `*/master` (or `*/main`)
- Script Path: `Jenkinsfile`

## 4) Webhook (optional but recommended)

In GitHub repo settings:
- Webhooks -> Add webhook
- Payload URL: `http://<your-server>:8081/github-webhook/`
- Content type: `application/json`
- Events: push + pull_request

## 5) Validate local CI script

```bash
cd /home/techmedaddy/projects/chronos
./scripts/ci/run-ci.sh
```

## Notes

- Pipeline runs containerized toolchain, so host CMake version mismatches won’t break builds.
- If Jenkins cannot access Docker, ensure Docker socket mount is present and permissions allow usage.
